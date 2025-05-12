#include "../../include/core/VideoDecoder.hpp"

#include <iostream>
#include <stdexcept>

// FFmpeg includes
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/error.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
#include <libswscale/swscale.h>
}

namespace VideoPlayer {
namespace Core {

// Helper function to convert FFmpeg error codes to string
std::string av_error_to_string(int errnum) {
    char errbuf[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(errnum, errbuf, AV_ERROR_MAX_STRING_SIZE);
    return std::string(errbuf);
}

VideoDecoder::VideoDecoder()
    : formatContext(nullptr),
      codecContext(nullptr),
      swsContext(nullptr),
      frame(nullptr),
      rgbFrame(nullptr),
      packet(nullptr),
      videoStreamIndex(-1),
      frameRate(0.0),
      timeBase(0.0),
      duration(0.0),
      currentPosition(0.0),
      width(0),
      height(0) {
    // Initialize FFmpeg network functionality
    // Note: av_register_all() is deprecated in newer FFmpeg versions
    static bool ffmpegNetworkInitialized = false;
    if (!ffmpegNetworkInitialized) {
        avformat_network_init();
        ffmpegNetworkInitialized = true;
    }
}

VideoDecoder::~VideoDecoder() {
    close();
}

bool VideoDecoder::open(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(mutex);

    // Close any existing decoder
    close();

    // Initialize decoder
    return initializeDecoder(filePath);
}

void VideoDecoder::close() {
    std::lock_guard<std::mutex> lock(mutex);

    // Clean up decoder
    cleanupDecoder();

    // Reset state
    videoStreamIndex = -1;
    frameRate = 0.0;
    timeBase = 0.0;
    duration = 0.0;
    currentPosition = 0.0;
    width = 0;
    height = 0;
    frameBuffer.clear();
}

bool VideoDecoder::isOpen() const {
    return formatContext != nullptr && codecContext != nullptr;
}

bool VideoDecoder::decodeNextFrame() {
    std::lock_guard<std::mutex> lock(mutex);

    if (!isOpen()) {
        return false;
    }

    // Read frames until we get a video frame
    while (readFrame()) {
        // Check if packet is from video stream
        if (packet->stream_index == videoStreamIndex) {
            // Decode packet
            if (decodePacket()) {
                // Convert frame to RGB
                if (convertFrame()) {
                    // Update position
                    updatePosition();
                    return true;
                }
            }
        }

        // Free packet
        av_packet_unref(packet);
    }

    return false;
}

bool VideoDecoder::seek(double position) {
    std::lock_guard<std::mutex> lock(mutex);

    if (!isOpen()) {
        return false;
    }

    // Convert position to stream time base
    int64_t timestamp = static_cast<int64_t>(position / timeBase);

    // Seek to timestamp
    int result = av_seek_frame(formatContext, videoStreamIndex, timestamp, AVSEEK_FLAG_BACKWARD);
    if (result < 0) {
        std::cerr << "Error seeking: " << av_error_to_string(result) << std::endl;
        return false;
    }

    // Flush codec buffers
    avcodec_flush_buffers(codecContext);

    // Update position
    currentPosition = position;

    return true;
}

const uint8_t* VideoDecoder::getFrameData() const {
    return frameBuffer.data();
}

int VideoDecoder::getWidth() const {
    return width;
}

int VideoDecoder::getHeight() const {
    return height;
}

double VideoDecoder::getFrameTime() const {
    return 1.0 / frameRate;
}

double VideoDecoder::getCurrentPosition() const {
    return currentPosition;
}

double VideoDecoder::getDuration() const {
    return duration;
}

int VideoDecoder::getStreamIndex() const {
    return videoStreamIndex;
}

double VideoDecoder::getFrameRate() const {
    return frameRate;
}

std::string VideoDecoder::getCodecName() const {
    if (!isOpen()) {
        return "";
    }

    return avcodec_get_name(codecContext->codec_id);
}

bool VideoDecoder::initializeDecoder(const std::string& filePath) {
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

        // Find video stream
        videoStreamIndex = -1;
        for (unsigned int i = 0; i < formatContext->nb_streams; i++) {
            if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                videoStreamIndex = i;
                break;
            }
        }

        if (videoStreamIndex == -1) {
            throw std::runtime_error("Could not find video stream");
        }

        // Get codec parameters
        AVCodecParameters* codecParams = formatContext->streams[videoStreamIndex]->codecpar;

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

        // Get video dimensions
        width = codecContext->width;
        height = codecContext->height;

        // Get frame rate
        AVRational frameRateRational = av_guess_frame_rate(formatContext, formatContext->streams[videoStreamIndex], nullptr);
        frameRate = static_cast<double>(frameRateRational.num) / frameRateRational.den;

        // Get time base
        timeBase =
            static_cast<double>(formatContext->streams[videoStreamIndex]->time_base.num) / formatContext->streams[videoStreamIndex]->time_base.den;

        // Get duration
        if (formatContext->duration != AV_NOPTS_VALUE) {
            duration = static_cast<double>(formatContext->duration) / AV_TIME_BASE;
        } else if (formatContext->streams[videoStreamIndex]->duration != AV_NOPTS_VALUE) {
            duration = static_cast<double>(formatContext->streams[videoStreamIndex]->duration) * timeBase;
        } else {
            duration = 0.0;
        }

        // Allocate frames
        frame = av_frame_alloc();
        if (!frame) {
            throw std::runtime_error("Could not allocate frame");
        }

        rgbFrame = av_frame_alloc();
        if (!rgbFrame) {
            throw std::runtime_error("Could not allocate RGB frame");
        }

        // Allocate packet
        packet = av_packet_alloc();
        if (!packet) {
            throw std::runtime_error("Could not allocate packet");
        }

        // Initialize frame buffer
        frameBuffer.resize(av_image_get_buffer_size(AV_PIX_FMT_RGBA, width, height, 1));

        // Set up RGB frame
        result = av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, frameBuffer.data(), AV_PIX_FMT_RGBA, width, height, 1);
        if (result < 0) {
            throw std::runtime_error("Could not set up RGB frame: " + av_error_to_string(result));
        }

        // Initialize swscale context
        swsContext = sws_getContext(width, height, codecContext->pix_fmt, width, height, AV_PIX_FMT_RGBA, SWS_BILINEAR, nullptr, nullptr, nullptr);

        if (!swsContext) {
            throw std::runtime_error("Could not initialize swscale context");
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error initializing decoder: " << e.what() << std::endl;

        // Clean up
        cleanupDecoder();

        return false;
    }
}

void VideoDecoder::cleanupDecoder() {
    // Free swscale context
    if (swsContext) {
        sws_freeContext(swsContext);
        swsContext = nullptr;
    }

    // Free packet
    if (packet) {
        av_packet_free(&packet);
        packet = nullptr;
    }

    // Free frames
    if (rgbFrame) {
        av_frame_free(&rgbFrame);
        rgbFrame = nullptr;
    }

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

bool VideoDecoder::readFrame() {
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

bool VideoDecoder::decodePacket() {
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

bool VideoDecoder::convertFrame() {
    if (!isOpen()) {
        return false;
    }

    // Convert frame to RGB
    int result = sws_scale(swsContext, frame->data, frame->linesize, 0, height, rgbFrame->data, rgbFrame->linesize);

    if (result < 0) {
        std::cerr << "Error converting frame: " << result << std::endl;
        return false;
    }

    return true;
}

void VideoDecoder::updatePosition() {
    if (!isOpen() || frame->pts == AV_NOPTS_VALUE) {
        return;
    }

    // Calculate current position
    currentPosition = static_cast<double>(frame->pts) * timeBase;
}

}  // namespace Core
}  // namespace VideoPlayer
