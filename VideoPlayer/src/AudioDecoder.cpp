#include "../include/AudioDecoder.hpp"

#include <iostream>
#include <vector>

AudioDecoder::AudioDecoder(const MediaFile& mediaFile)
    : audioCodecContext_(nullptr),
      mediaFile_(mediaFile) {

    isRunning_ = false;
    isPaused_ = false;

    int audioStreamIndex = mediaFile_.GetAudioStreamIndex();
    if (audioStreamIndex < 0) {
        throw AudioDecoderError("No audio stream found");
    }

    AVStream* audioStream = mediaFile_.GetFormatContext()->streams[audioStreamIndex];

    // Find decoder
    const AVCodec* codec = avcodec_find_decoder(audioStream->codecpar->codec_id);
    if (!codec) {
        throw AudioDecoderError("Unsupported audio codec");
    }

    // Allocate codec context
    audioCodecContext_ = avcodec_alloc_context3(codec);
    if (!audioCodecContext_) {
        throw AudioDecoderError("Could not allocate audio codec context");
    }

    // Copy codec parameters
    if (avcodec_parameters_to_context(audioCodecContext_, audioStream->codecpar) < 0) {
        throw AudioDecoderError("Could not copy audio codec parameters");
    }

    // Open codec
    if (avcodec_open2(audioCodecContext_, codec, nullptr) < 0) {
        throw AudioDecoderError("Could not open audio codec");
    }
}

AudioDecoder::~AudioDecoder() {
    Stop();

    if (audioCodecContext_) {
        avcodec_free_context(&audioCodecContext_);
    }
}

void AudioDecoder::SetVolume(float volume) {
    sound_.setVolume(volume);
}

void AudioDecoder::Start() {
    if (!isRunning_) {
        isRunning_ = true;
        isPaused_ = false;
        std::thread decodeThread(&AudioDecoder::DecodeAudio, this);
        decodeThread.detach();
    }
}

void AudioDecoder::Flush() {
    if (audioCodecContext_) {
        avcodec_flush_buffers(audioCodecContext_);
    }
}

void AudioDecoder::DecodeAudio() {
    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();

    if (!packet || !frame) {
        std::cerr << "Could not allocate packet or frame" << std::endl;
        if (packet) av_packet_free(&packet);
        if (frame) av_frame_free(&frame);
        return;
    }

    AVFormatContext* formatContext = mediaFile_.GetFormatContext();
    int audioStreamIndex = mediaFile_.GetAudioStreamIndex();
    double audioTimeBase = mediaFile_.GetAudioTimeBase();

    // Prepare audio buffer
    std::vector<int16_t> audioBuffer;

    while (isRunning_ && av_read_frame(formatContext, packet) >= 0) {
        if (packet->stream_index == audioStreamIndex) {
            int response = avcodec_send_packet(audioCodecContext_, packet);

            if (response < 0) {
                std::cerr << "Error sending packet for audio decoding" << std::endl;
                break;
            }

            while (response >= 0) {
                response = avcodec_receive_frame(audioCodecContext_, frame);

                if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
                    break;
                } else if (response < 0) {
                    std::cerr << "Error during audio decoding" << std::endl;
                    isRunning_ = false;
                    break;
                }

                // Process audio data
                int numSamples = frame->nb_samples;
                int channels = audioCodecContext_->channels;

                // Resize buffer to fit new samples
                size_t oldSize = audioBuffer.size();
                audioBuffer.resize(oldSize + numSamples * channels);

                // Convert audio to int16_t format
                for (int i = 0; i < numSamples; i++) {
                    for (int ch = 0; ch < channels; ch++) {
                        if (audioCodecContext_->sample_fmt == AV_SAMPLE_FMT_FLTP) {
                            float sample = ((float*)frame->data[ch])[i];
                            audioBuffer[oldSize + i * channels + ch] = (int16_t)(sample * 32767);
                        } else if (audioCodecContext_->sample_fmt == AV_SAMPLE_FMT_S16) {
                            audioBuffer[oldSize + i * channels + ch] = ((int16_t*)frame->data[0])[i * channels + ch];
                        }
                    }
                }

                // Load audio data into SFML sound buffer when we have enough data
                if (audioBuffer.size() >= audioCodecContext_->sample_rate * channels) {
                    std::lock_guard<std::mutex> lock(mutex_);

                    if (soundBuffer_.loadFromSamples(
                        audioBuffer.data(),
                        audioBuffer.size(),
                        channels,
                        audioCodecContext_->sample_rate)) {

                        sound_.setBuffer(soundBuffer_);
                        sound_.play();

                        // Clear buffer after loading
                        audioBuffer.clear();
                    }
                }

                // Wait if paused
                while (isPaused_ && isRunning_) {
                    sound_.pause();
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }

                if (!isPaused_ && sound_.getStatus() == sf::Sound::Paused) {
                    sound_.play();
                }

                if (!isRunning_) {
                    break;
                }
            }
        }

        av_packet_unref(packet);
    }

    av_packet_free(&packet);
    av_frame_free(&frame);

    isRunning_ = false;
}
