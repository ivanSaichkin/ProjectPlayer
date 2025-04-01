#include "../include/VideoDecoder.hpp"

#include <iostream>

VideoDecoder::VideoDecoder(const MediaFile& mediaFile)
    : videoCodecContext_(nullptr), videoFrame_(nullptr), swsContext_(nullptr), mediaFile_(mediaFile) {
    isRunning_ = false;
    isPaused_ = false;
    endOfStream_ = false;

    int videoStreamIndex = mediaFile_.GetVideoStreamIndex();
    if (videoStreamIndex < 0) {
        throw VideoDecoderError("No video stream found");
    }

    AVStream* videoStream = mediaFile_.GetFormatContext()->streams[videoStreamIndex];

    // Find decoder
    const AVCodec* codec = avcodec_find_decoder(videoStream->codecpar->codec_id);
    if (!codec) {
        throw VideoDecoderError("Unsupported video codec");
    }

    // Allocate codec context
    videoCodecContext_ = avcodec_alloc_context3(codec);
    if (!videoCodecContext_) {
        throw VideoDecoderError("Could not allocate video codec context");
    }

    // Copy codec parameters
    if (avcodec_parameters_to_context(videoCodecContext_, videoStream->codecpar) < 0) {
        throw VideoDecoderError("Could not copy video codec parameters");
    }

    // Open codec
    if (avcodec_open2(videoCodecContext_, codec, nullptr) < 0) {
        throw VideoDecoderError("Could not open video codec");
    }

    // Allocate video frame
    videoFrame_ = av_frame_alloc();
    if (!videoFrame_) {
        throw VideoDecoderError("Could not allocate video frame");
    }

    // Create scaling context
    swsContext_ = sws_getContext(videoCodecContext_->width, videoCodecContext_->height, videoCodecContext_->pix_fmt, videoCodecContext_->width,
                                 videoCodecContext_->height, AV_PIX_FMT_RGBA, SWS_BILINEAR, nullptr, nullptr, nullptr);

    if (!swsContext_) {
        throw VideoDecoderError("Could not initialize scaling context");
    }

    // Create SFML texture
    if (!texture_.create(videoCodecContext_->width, videoCodecContext_->height)) {
        throw VideoDecoderError("Could not create texture");
    }

    sprite_.setTexture(texture_);
}

VideoDecoder::~VideoDecoder() {
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

    if (videoFrame_) {
        av_frame_free(&videoFrame_);
    }

    if (videoCodecContext_) {
        avcodec_free_context(&videoCodecContext_);
    }

    if (swsContext_) {
        sws_freeContext(swsContext_);
    }
}

void VideoDecoder::Start() {
    if (!isRunning_) {
        isRunning_ = true;
        isPaused_ = false;
        endOfStream_ = false;
        std::thread decodeThread(&VideoDecoder::DecodeVideo, this);
        decodeThread.detach();
    }
}

void VideoDecoder::Flush() {
    if (videoCodecContext_) {
        avcodec_flush_buffers(videoCodecContext_);
    }

    // Очищаем очередь пакетов
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        while (!packetQueue_.empty()) {
            AVPacket* packet = packetQueue_.front();
            packetQueue_.pop();
            av_packet_free(&packet);
        }
    }

    endOfStream_ = false;

    // Сбрасываем внутреннее состояние
    std::lock_guard<std::mutex> lock(mutex_);

    sf::Vector2u textureSize = texture_.getSize();
    std::vector<sf::Uint8> blackPixels(textureSize.x * textureSize.y * 4, 0);
    texture_.update(blackPixels.data());
}

void VideoDecoder::Draw(sf::RenderWindow& window) {
    std::lock_guard<std::mutex> lock(mutex_);
    window.draw(sprite_);
}

void VideoDecoder::ProcessPacket(AVPacket* packet) {
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

void VideoDecoder::SignalEndOfStream() {
    endOfStream_ = true;
    packetCondition_.notify_one();
}

void VideoDecoder::ProcessVideoFrame(AVFrame* frame) {
    double pts = frame->pts * mediaFile_.GetVideoTimeBase();

    // Allocate RGBA frame
    AVFrame* rgbaFrame = av_frame_alloc();
    if (!rgbaFrame) {
        std::cerr << "Could not allocate RGBA frame" << std::endl;
        return;
    }

    // Allocate buffer for RGBA frame
    int bufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGBA, videoCodecContext_->width, videoCodecContext_->height, 1);

    uint8_t* buffer = (uint8_t*)av_malloc(bufferSize);
    if (!buffer) {
        std::cerr << "Could not allocate buffer for RGBA frame" << std::endl;
        av_frame_free(&rgbaFrame);
        return;
    }

    av_image_fill_arrays(rgbaFrame->data, rgbaFrame->linesize, buffer, AV_PIX_FMT_RGBA, videoCodecContext_->width, videoCodecContext_->height, 1);

    // Convert frame to RGBA
    sws_scale(swsContext_, frame->data, frame->linesize, 0, videoCodecContext_->height, rgbaFrame->data, rgbaFrame->linesize);

    // Wait for the correct time to display this frame
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto currentTime = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(currentTime - startTime_).count();

        if (pts > elapsed) {
            std::this_thread::sleep_for(std::chrono::duration<double>(pts - elapsed));
        }

        // Update texture with new frame
        texture_.update(rgbaFrame->data[0]);
    }

    // Clean up
    av_free(buffer);
    av_frame_free(&rgbaFrame);
}

void VideoDecoder::DecodeVideo() {
    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        std::cerr << "Could not allocate frame" << std::endl;
        return;
    }

    while (isRunning_) {
        // Wait if paused
        while (isPaused_ && isRunning_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        if (!isRunning_) {
            break;
        }

        // Get packet from queue
        AVPacket* packet = nullptr;
        bool gotPacket = false;

        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            packetCondition_.wait(lock, [this] { return !packetQueue_.empty() || endOfStream_ || !isRunning_; });

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
            int response = avcodec_send_packet(videoCodecContext_, packet);
            av_packet_free(&packet);

            if (response < 0) {
                std::cerr << "Error sending packet for decoding" << std::endl;
                continue;
            }

            // Receive frames
            while (response >= 0) {
                response = avcodec_receive_frame(videoCodecContext_, frame);

                if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
                    break;
                } else if (response < 0) {
                    std::cerr << "Error during decoding" << std::endl;
                    break;
                }

                // Process the frame
                ProcessVideoFrame(frame);
            }
        } else if (endOfStream_) {
            // Flush the decoder
            avcodec_send_packet(videoCodecContext_, nullptr);

            while (true) {
                int response = avcodec_receive_frame(videoCodecContext_, frame);
                if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
                    break;
                } else if (response < 0) {
                    std::cerr << "Error during flushing" << std::endl;
                    break;
                }

                // Process the frame
                ProcessVideoFrame(frame);
            }

            // Exit the loop when all frames are processed
            if (packetQueue_.empty()) {
                break;
            }
        }
    }

    av_frame_free(&frame);
    isRunning_ = false;
}

sf::Sprite& VideoDecoder::GetSprite() {
    return sprite_;
}

sf::Vector2i VideoDecoder::GetSize() const {
    if (videoCodecContext_) {
        return sf::Vector2i(videoCodecContext_->width, videoCodecContext_->height);
    }
    return sf::Vector2i(0, 0);
}
