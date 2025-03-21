#include "../include/VideoDecoder.hpp"

#include <iostream>

VideoDecoder::VideoDecoder(const MediaFile& mediaFile)
    : videoCodecContext_(nullptr),
      videoFrame_(nullptr),
      swsContext_(nullptr),
      mediaFile_(mediaFile) {

    isRunning_ = false;
    isPaused_ = false;

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
    swsContext_ = sws_getContext(
        videoCodecContext_->width,
        videoCodecContext_->height,
        videoCodecContext_->pix_fmt,
        videoCodecContext_->width,
        videoCodecContext_->height,
        AV_PIX_FMT_RGBA,
        SWS_BILINEAR,
        nullptr,
        nullptr,
        nullptr
    );

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
        std::thread decodeThread(&VideoDecoder::DecodeVideo, this);
        decodeThread.detach();
    }
}

void VideoDecoder::Flush() {
    if (videoCodecContext_) {
        avcodec_flush_buffers(videoCodecContext_);
    }
}

void VideoDecoder::Draw(sf::RenderWindow& window) {
    std::lock_guard<std::mutex> lock(mutex_);
    window.draw(sprite_);
}

void VideoDecoder::DecodeVideo() {
    AVPacket* packet = av_packet_alloc();
    AVFrame* rgbaFrame = av_frame_alloc();

    if (!packet || !rgbaFrame) {
        std::cerr << "Could not allocate packet or RGBA frame" << std::endl;
        if (packet) av_packet_free(&packet);
        if (rgbaFrame) av_frame_free(&rgbaFrame);
        return;
    }

    // Allocate buffer for RGBA frame
    int bufferSize = av_image_get_buffer_size(
        AV_PIX_FMT_RGBA,
        videoCodecContext_->width,
        videoCodecContext_->height,
        1
    );

    uint8_t* buffer = (uint8_t*)av_malloc(bufferSize);
    if (!buffer) {
        std::cerr << "Could not allocate buffer for RGBA frame" << std::endl;
        av_packet_free(&packet);
        av_frame_free(&rgbaFrame);
        return;
    }

    av_image_fill_arrays(
        rgbaFrame->data,
        rgbaFrame->linesize,
        buffer,
        AV_PIX_FMT_RGBA,
        videoCodecContext_->width,
        videoCodecContext_->height,
        1
    );

    AVFormatContext* formatContext = mediaFile_.GetFormatContext();
    int videoStreamIndex = mediaFile_.GetVideoStreamIndex();
    double videoTimeBase = mediaFile_.GetVideoTimeBase();

    while (isRunning_ && av_read_frame(formatContext, packet) >= 0) {
        if (packet->stream_index == videoStreamIndex) {
            int response = avcodec_send_packet(videoCodecContext_, packet);

            if (response < 0) {
                std::cerr << "Error sending packet for decoding" << std::endl;
                break;
            }

            while (response >= 0) {
                response = avcodec_receive_frame(videoCodecContext_, videoFrame_);

                if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
                    break;
                } else if (response < 0) {
                    std::cerr << "Error during decoding" << std::endl;
                    isRunning_ = false;
                    break;
                }

                // Convert frame to RGBA
                sws_scale(
                    swsContext_,
                    videoFrame_->data,
                    videoFrame_->linesize,
                    0,
                    videoCodecContext_->height,
                    rgbaFrame->data,
                    rgbaFrame->linesize
                );

                // Calculate frame timing
                double pts = videoFrame_->pts * videoTimeBase;

                {
                    std::lock_guard<std::mutex> lock(mutex_);

                    // Wait for the correct time to display this frame
                    auto currentTime = std::chrono::steady_clock::now();
                    double elapsed = std::chrono::duration<double>(currentTime - startTime_).count();

                    if (pts > elapsed) {
                        std::this_thread::sleep_for(std::chrono::duration<double>(pts - elapsed));
                    }

                    // Update texture with new frame
                    texture_.update(rgbaFrame->data[0]);
                }

                // Wait if paused
                while (isPaused_ && isRunning_) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }

                if (!isRunning_) {
                    break;
                }
            }
        }

        av_packet_unref(packet);
    }

    av_free(buffer);
    av_packet_free(&packet);
    av_frame_free(&rgbaFrame);

    isRunning_ = false;
}
