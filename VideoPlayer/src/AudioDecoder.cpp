#include "../include/AudioDecoder.hpp"

#include <iostream>

AudioDecoder::AudioDecoder(const MediaFile& mediaFile) : audioCodecContext_(nullptr), mediaFile_(mediaFile), volume_(50.0f) {
    isRunning_ = false;
    isPaused_ = false;
    endOfStream_ = false;

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

    // Initialize sound
    sound_.setVolume(volume_);
}

AudioDecoder::~AudioDecoder() {
    Stop();

    // Clear packet queue
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        while (!packetQueue_.empty()) {
            AVPacket* packet = packetQueue_.front();
            packetQueue_.pop();
            av_packet_free(&packet);
        }
    }

    if (audioCodecContext_) {
        avcodec_free_context(&audioCodecContext_);
    }
}

void AudioDecoder::SetVolume(float volume) {
    volume_ = volume;
    sound_.setVolume(volume);
}

float AudioDecoder::GetVolume() const {
    return volume_;
}

void AudioDecoder::Start() {
    if (!isRunning_) {
        isRunning_ = true;
        isPaused_ = false;
        endOfStream_ = false;
        audioBuffer_.clear();
        std::thread decodeThread(&AudioDecoder::DecodeAudio, this);
        decodeThread.detach();
    }
}

void AudioDecoder::Flush() {
    endOfStream_ = false;
    if (audioCodecContext_) {
        avcodec_flush_buffers(audioCodecContext_);
    }

    // Clear packet queue
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        while (!packetQueue_.empty()) {
            AVPacket* packet = packetQueue_.front();
            packetQueue_.pop();
            av_packet_free(&packet);
        }
    }

    audioBuffer_.clear();
    endOfStream_ = false;

    // Stop current sound
    sound_.stop();
}

void AudioDecoder::ProcessPacket(AVPacket* packet) {
    if (!isRunning_)
        return;

    // Make a copy of the packet
    AVPacket* packetCopy = av_packet_alloc();
    if (!packetCopy) {
        std::cerr << "Could not allocate packet" << std::endl;
        return;
    }

    if (av_packet_ref(packetCopy, packet) < 0) {
        std::cerr << "Could not reference packet" << std::endl;
        av_packet_free(&packetCopy);
        return;
    }

    // Add packet to queue
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        packetQueue_.push(packetCopy);
    }

    // Notify decoder thread
    packetCondition_.notify_one();
}

void AudioDecoder::SignalEndOfStream() {
    endOfStream_ = true;
    packetCondition_.notify_one();
}

void AudioDecoder::ProcessAudioFrame(AVFrame* frame) {
    // Process audio data
    int numSamples = frame->nb_samples;
    int channels = audioCodecContext_->channels;

    // Resize buffer to fit new samples
    size_t oldSize = audioBuffer_.size();
    audioBuffer_.resize(oldSize + numSamples * channels);

    // Convert audio to int16_t format
    for (int i = 0; i < numSamples; i++) {
        for (int ch = 0; ch < channels; ch++) {
            if (audioCodecContext_->sample_fmt == AV_SAMPLE_FMT_FLTP) {
                float sample = ((float*)frame->data[ch])[i];
                audioBuffer_[oldSize + i * channels + ch] = (int16_t)(sample * 32767);
            } else if (audioCodecContext_->sample_fmt == AV_SAMPLE_FMT_S16 || audioCodecContext_->sample_fmt == AV_SAMPLE_FMT_S16P) {
                if (audioCodecContext_->sample_fmt == AV_SAMPLE_FMT_S16) {
                    audioBuffer_[oldSize + i * channels + ch] = ((int16_t*)frame->data[0])[i * channels + ch];
                } else {  // S16P
                    audioBuffer_[oldSize + i * channels + ch] = ((int16_t*)frame->data[ch])[i];
                }
            }
        }
    }

    // Load audio data into SFML sound buffer when we have enough data
    if (audioBuffer_.size() >= audioCodecContext_->sample_rate * channels * 1024) {  // Use smaller chunks
        std::lock_guard<std::mutex> lock(mutex_);

        if (soundBuffer_.loadFromSamples(audioBuffer_.data(), audioBuffer_.size(), channels, audioCodecContext_->sample_rate)) {
            sound_.setBuffer(soundBuffer_);

            if (isPaused_) {
                sound_.pause();
            } else {
                sound_.play();
            }

            // Clear buffer after loading
            audioBuffer_.clear();
        }
    }
}

void AudioDecoder::DecodeAudio() {
    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        std::cerr << "Could not allocate frame" << std::endl;
        return;
    }

    while (isRunning_) {
        // Check sound status
        if (sound_.getStatus() == sf::Sound::Stopped && !audioBuffer_.empty()) {
            // If sound has stopped but we have more data, play it
            std::lock_guard<std::mutex> lock(mutex_);

            if (soundBuffer_.loadFromSamples(audioBuffer_.data(), audioBuffer_.size(), audioCodecContext_->channels,
                                             audioCodecContext_->sample_rate)) {
                sound_.setBuffer(soundBuffer_);

                if (!isPaused_) {
                    sound_.play();
                }

                audioBuffer_.clear();
            }
        }

        // Handle pause state
        if (isPaused_) {
            if (sound_.getStatus() == sf::Sound::Playing) {
                sound_.pause();
            }
        } else if (sound_.getStatus() == sf::Sound::Paused) {
            sound_.play();
        }

        // Get packet from queue
        AVPacket* packet = nullptr;
        bool gotPacket = false;

        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            packetCondition_.wait_for(lock, std::chrono::milliseconds(100), [this] { return !packetQueue_.empty() || endOfStream_ || !isRunning_; });

            if (!isRunning_) {
                break;
            }

            if (!packetQueue_.empty()) {
                packet = packetQueue_.front();
                packetQueue_.pop();
                gotPacket = true;
            }
        }

        // Process packet
        if (gotPacket) {
            int response = avcodec_send_packet(audioCodecContext_, packet);
            av_packet_free(&packet);

            if (response < 0) {
                std::cerr << "Error sending packet for audio decoding" << std::endl;
                continue;
            }

            // Receive frames
            while (response >= 0) {
                response = avcodec_receive_frame(audioCodecContext_, frame);

                if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
                    break;
                } else if (response < 0) {
                    std::cerr << "Error during audio decoding" << std::endl;
                    break;
                }

                // Process the frame
                ProcessAudioFrame(frame);
            }
        } else if (endOfStream_ && audioBuffer_.empty() && sound_.getStatus() == sf::Sound::Stopped) {
            // Flush the decoder
            avcodec_send_packet(audioCodecContext_, nullptr);

            while (true) {
                int response = avcodec_receive_frame(audioCodecContext_, frame);
                if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
                    break;
                } else if (response < 0) {
                    std::cerr << "Error during audio flushing" << std::endl;
                    break;
                }

                // Process the frame
                ProcessAudioFrame(frame);
            }

            if (!audioBuffer_.empty()) {
                std::lock_guard<std::mutex> lock(mutex_);
                if (soundBuffer_.loadFromSamples(audioBuffer_.data(), audioBuffer_.size(), audioCodecContext_->channels,
                                                 audioCodecContext_->sample_rate)) {
                    sound_.setBuffer(soundBuffer_);
                    if (!isPaused_)
                        sound_.play();
                    audioBuffer_.clear();
                }
            }

            // Exit the loop when all frames are processed
            if (packetQueue_.empty() && audioBuffer_.empty() && sound_.getStatus() == sf::Sound::Stopped) {
                break;
            }
        }
    }

    av_frame_free(&frame);
    isRunning_ = false;
}
