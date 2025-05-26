#include "../../include/core/AudioDecoder.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>

// FFmpeg includes
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/error.h>
#include <libavutil/opt.h>
#include <libavutil/time.h>
#include <libswresample/swresample.h>
}

namespace VideoPlayer {
namespace Core {

// Helper function to convert FFmpeg error codes to string
inline std::string av_error_to_string(int errnum) {
    char errbuf[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(errnum, errbuf, AV_ERROR_MAX_STRING_SIZE);
    return std::string(errbuf);
}

AudioDecoder::AudioDecoder()
    : formatContext(nullptr),
      codecContext(nullptr),
      swrContext(nullptr),
      frame(nullptr),
      packet(nullptr),
      audioStreamIndex(-1),
      timeBase(0.0),
      duration(0.0),
      currentPosition(0.0),
      sampleRate(0),
      channels(0),
      bufferPosition(0) {
    // Initialize FFmpeg network functionality
    static bool ffmpegNetworkInitialized = false;
    if (!ffmpegNetworkInitialized) {
        avformat_network_init();
        ffmpegNetworkInitialized = true;
    }
}

AudioDecoder::~AudioDecoder() {
    close();
}

bool AudioDecoder::open(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(mutex);

    // Close any existing decoder
    close();

    // Initialize decoder
    return initializeDecoder(filePath);
}

void AudioDecoder::close() {
    std::lock_guard<std::mutex> lock(mutex);

    // Clean up decoder
    cleanupDecoder();

    // Reset state
    audioStreamIndex = -1;
    timeBase = 0.0;
    duration = 0.0;
    currentPosition = 0.0;
    sampleRate = 0;
    channels = 0;
    audioBuffer.clear();
    bufferPosition = 0;
}

bool AudioDecoder::isOpen() const {
    return formatContext != nullptr && codecContext != nullptr;
}

bool AudioDecoder::decodeAudioSamples(std::vector<short>& samples, int numSamples) {
    std::lock_guard<std::mutex> lock(mutex);

    if (!isOpen()) {
        return false;
    }

    // Clear output samples
    samples.clear();

    // Check if we have enough samples in the buffer
    if (audioBuffer.size() - bufferPosition >= static_cast<size_t>(numSamples)) {
        // Copy samples from buffer
        samples.insert(samples.end(), audioBuffer.begin() + bufferPosition, audioBuffer.begin() + bufferPosition + numSamples);

        // Update buffer position
        bufferPosition += numSamples;

        // Clear buffer if we've used all samples
        if (bufferPosition >= audioBuffer.size()) {
            audioBuffer.clear();
            bufferPosition = 0;
        }

        return true;
    }

    // Copy remaining samples from buffer
    if (bufferPosition < audioBuffer.size()) {
        samples.insert(samples.end(), audioBuffer.begin() + bufferPosition, audioBuffer.end());

        // Clear buffer
        audioBuffer.clear();
        bufferPosition = 0;
    }

    // Decode more samples
    while (samples.size() < static_cast<size_t>(numSamples)) {
        // Read frames until we get an audio frame
        bool frameDecoded = false;
        while (!frameDecoded && readFrame()) {
            // Check if packet is from audio stream
            if (packet->stream_index == audioStreamIndex) {
                // Decode packet
                if (decodePacket()) {
                    // Convert frame to PCM samples
                    std::vector<short> frameSamples;
                    if (convertFrame(frameSamples)) {
                        // Update position
                        updatePosition();

                        // Add samples to output
                        samples.insert(samples.end(), frameSamples.begin(), frameSamples.end());

                        frameDecoded = true;
                    }
                }
            }

            // Free packet
            av_packet_unref(packet);
        }

        // If we couldn't decode a frame, we've reached the end of the stream
        if (!frameDecoded) {
            break;
        }
    }

    // If we have more samples than requested, store the excess in the buffer
    if (samples.size() > static_cast<size_t>(numSamples)) {
        audioBuffer.insert(audioBuffer.end(), samples.begin() + numSamples, samples.end());

        // Truncate output samples
        samples.resize(numSamples);

        bufferPosition = 0;
    }

    return !samples.empty();
}

bool AudioDecoder::seek(double position) {
    std::lock_guard<std::mutex> lock(mutex);

    if (!isOpen()) {
        return false;
    }

    // Convert position to stream time base
    int64_t timestamp = static_cast<int64_t>(position / timeBase);

    // Seek to timestamp
    int result = av_seek_frame(formatContext, audioStreamIndex, timestamp, AVSEEK_FLAG_BACKWARD);
    if (result < 0) {
        std::cerr << "Error seeking: " << av_error_to_string(result) << std::endl;
        return false;
    }

    // Flush codec buffers
    avcodec_flush_buffers(codecContext);

    // Clear audio buffer
    audioBuffer.clear();
    bufferPosition = 0;

    // Update position
    currentPosition = position;

    return true;
}

int AudioDecoder::getSampleRate() const {
    return sampleRate;
}

int AudioDecoder::getChannels() const {
    return channels;
}

double AudioDecoder::getCurrentPosition() const {
    return currentPosition;
}

double AudioDecoder::getDuration() const {
    return duration;
}

int AudioDecoder::getStreamIndex() const {
    return audioStreamIndex;
}

std::string AudioDecoder::getCodecName() const {
    if (!isOpen()) {
        return "";
    }

    return avcodec_get_name(codecContext->codec_id);
}

bool AudioDecoder::initializeDecoder(const std::string& filePath) {
    try {
        // Open input file
        int result = avformat_open_input(&formatContext, filePath.c_str(), nullptr, nullptr);
        if (result < 0) {
            throw std::runtime_error("Could not open input file: " + av_error_to_string(result));
        }

        // Find stream info
        result = avformat_find_stream_info(formatContext, nullptr);
        if (result < 0) {
            throw std::runtime_error("Could not find stream info: " + av_error_to_string(result));
        }

        // Find audio stream
        audioStreamIndex = -1;
        for (unsigned int i = 0; i < formatContext->nb_streams; i++) {
            if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                audioStreamIndex = i;
                break;
            }
        }

        if (audioStreamIndex == -1) {
            throw std::runtime_error("Could not find audio stream");
        }

        // Get codec parameters
        AVCodecParameters* codecParams = formatContext->streams[audioStreamIndex]->codecpar;

        // Find decoder
        const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);
        if (!codec) {
            throw std::runtime_error("Unsupported codec");
        }

        // Allocate codec context
        codecContext = avcodec_alloc_context3(codec);
        if (!codecContext) {
            throw std::runtime_error("Could not allocate codec context");
        }

        // Copy codec parameters to codec context
        result = avcodec_parameters_to_context(codecContext, codecParams);
        if (result < 0) {
            throw std::runtime_error("Could not copy codec parameters: " + av_error_to_string(result));
        }

        // Open codec
        result = avcodec_open2(codecContext, codec, nullptr);
        if (result < 0) {
            throw std::runtime_error("Could not open codec: " + av_error_to_string(result));
        }

        // Get audio parameters
        sampleRate = codecContext->sample_rate;
        channels = codecContext->channels;

        // Get time base
        timeBase =
            static_cast<double>(formatContext->streams[audioStreamIndex]->time_base.num) / formatContext->streams[audioStreamIndex]->time_base.den;

        // Get duration
        if (formatContext->duration != AV_NOPTS_VALUE) {
            duration = static_cast<double>(formatContext->duration) / AV_TIME_BASE;
        } else if (formatContext->streams[audioStreamIndex]->duration != AV_NOPTS_VALUE) {
            duration = static_cast<double>(formatContext->streams[audioStreamIndex]->duration) * timeBase;
        } else {
            duration = 0.0;
        }

        // Allocate frame
        frame = av_frame_alloc();
        if (!frame) {
            throw std::runtime_error("Could not allocate frame");
        }

        // Allocate packet
        packet = av_packet_alloc();
        if (!packet) {
            throw std::runtime_error("Could not allocate packet");
        }

        // Initialize resampler
        swrContext = swr_alloc();
        if (!swrContext) {
            throw std::runtime_error("Could not allocate resampler context");
        }

        // Set resampler options
        av_opt_set_int(swrContext, "in_channel_layout", codecContext->channel_layout, 0);
        av_opt_set_int(swrContext, "out_channel_layout", AV_CH_LAYOUT_STEREO, 0);
        av_opt_set_int(swrContext, "in_sample_rate", codecContext->sample_rate, 0);
        av_opt_set_int(swrContext, "out_sample_rate", codecContext->sample_rate, 0);
        av_opt_set_sample_fmt(swrContext, "in_sample_fmt", codecContext->sample_fmt, 0);
        av_opt_set_sample_fmt(swrContext, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);

        // Initialize resampler
        result = swr_init(swrContext);
        if (result < 0) {
            throw std::runtime_error("Could not initialize resampler: " + av_error_to_string(result));
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error initializing decoder: " << e.what() << std::endl;

        // Clean up
        cleanupDecoder();

        return false;
    }
}

void AudioDecoder::cleanupDecoder() {
    // Free resampler context
    if (swrContext) {
        swr_free(&swrContext);
        swrContext = nullptr;
    }

    // Free packet
    if (packet) {
        av_packet_free(&packet);
        packet = nullptr;
    }

    // Free frame
    if (frame) {
        av_frame_free(&frame);
        frame = nullptr;
    }

    // Close codec context
    if (codecContext) {
        avcodec_free_context(&codecContext);
        codecContext = nullptr;
    }

    // Close format context
    if (formatContext) {
        avformat_close_input(&formatContext);
        formatContext = nullptr;
    }
}

bool AudioDecoder::readFrame() {
    if (!isOpen()) {
        return false;
    }

    // Free previous packet
    av_packet_unref(packet);

    // Read next packet
    int result = av_read_frame(formatContext, packet);
    if (result < 0) {
        return false;
    }

    return true;
}

bool AudioDecoder::decodePacket() {
    if (!isOpen()) {
        return false;
    }

    // Send packet to decoder
    int result = avcodec_send_packet(codecContext, packet);
    if (result < 0) {
        std::cerr << "Error sending packet to decoder: " << av_error_to_string(result) << std::endl;
        return false;
    }

    // Receive frame from decoder
    result = avcodec_receive_frame(codecContext, frame);
    if (result < 0) {
        if (result != AVERROR(EAGAIN) && result != AVERROR_EOF) {
            std::cerr << "Error receiving frame from decoder: " << av_error_to_string(result) << std::endl;
        }
        return false;
    }

    return true;
}

bool AudioDecoder::convertFrame(std::vector<short>& outSamples) {
    if (!isOpen()) {
        return false;
    }

    // Calculate number of samples
    int numSamples = frame->nb_samples * channels;

    // Resize output buffer
    outSamples.resize(numSamples);

    // Convert audio samples
    uint8_t* outBuffer = reinterpret_cast<uint8_t*>(outSamples.data());

    // Convert samples
    int result = swr_convert(swrContext, &outBuffer, frame->nb_samples, (const uint8_t**)frame->data, frame->nb_samples);

    if (result < 0) {
        std::cerr << "Error converting audio: " << av_error_to_string(result) << std::endl;
        return false;
    }

    // Resize output to actual number of converted samples
    outSamples.resize(result * channels);

    return true;
}

void AudioDecoder::updatePosition() {
    if (!isOpen() || frame->pts == AV_NOPTS_VALUE) {
        return;
    }

    // Calculate current position
    currentPosition = static_cast<double>(frame->pts) * timeBase;
}

}  // namespace Core
}  // namespace VideoPlayer
