#include "../include/VideoDecoder.hpp"

VideoDecoder::VideoDecoder(const MediaFile& mediaFile)
    : mediaFile_(mediaFile), videoCodecContext_(nullptr), swsContext_(nullptr){

    isPaused_ = false;
    isRunning_ = false;

    AVCodecParameters* codecParams = mediaFile_.GetFormatContext()->streams[mediaFile_.GetVideoStreamIndex()]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);

    if (!codec) throw std::runtime_error("Unsupported video codec");

    videoCodecContext_ = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(videoCodecContext_, codecParams);

    if (avcodec_open2(videoCodecContext_, codec, nullptr) < 0)
        throw std::runtime_error("Failed to open video codec");

    swsContext_ = sws_getContext(videoCodecContext_->width, videoCodecContext_->height, videoCodecContext_->pix_fmt,
        videoCodecContext_->width, videoCodecContext_->height, AV_PIX_FMT_RGB32,
                                  SWS_BILINEAR, nullptr, nullptr, nullptr);

    texture_.create(videoCodecContext_->width, videoCodecContext_->height);
}

VideoDecoder::~VideoDecoder() {
    Stop();
    if (videoFrame_) {
        av_frame_free(&videoFrame_);
    }
    if (swsContext_) {
        sws_freeContext(swsContext_);
    }
    if (videoCodecContext_) {
        avcodec_free_context(&videoCodecContext_);
    }
}

void VideoDecoder::Start() {
    isRunning_ = true;
    std::thread([this]() { DecodeVideo(); }).detach();
}

void VideoDecoder::Stop() {
    isRunning_ = false;
}

void VideoDecoder::Draw(sf::RenderWindow& window) {
    window.draw(sprite_);
}

void VideoDecoder::TogglePause() {
    isRunning_ =!isRunning_;
}

void VideoDecoder::DecodeVideo() {
    AVPacket* packet = av_packet_alloc();
    if (!packet) {
        throw std::runtime_error("Failed to allocate AVPacket");
    }

    while (isRunning_) {
        if (isPaused_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        if (av_read_frame(mediaFile_.GetFormatContext(), packet) >= 0) {
            if (packet->stream_index == mediaFile_.GetVideoStreamIndex()) {
                if (avcodec_send_packet(videoCodecContext_, packet) == 0) {
                    AVFrame* frame = av_frame_alloc();
                    if (!frame) {
                        throw std::runtime_error("Failed to allocate AVFrame");
                    }

                    while (avcodec_receive_frame(videoCodecContext_, frame) == 0) {
                        uint8_t* pixels = new uint8_t[videoCodecContext_->width * videoCodecContext_->height * 4];
                        uint8_t* dest[4] = {pixels, nullptr, nullptr, nullptr};
                        int destLinesize[4] = {videoCodecContext_->width * 4, 0, 0, 0};

                        sws_scale(swsContext_, frame->data, frame->linesize, 0, videoCodecContext_->height, dest, destLinesize);

                        texture_.update(pixels);
                        sprite_.setTexture(texture_);

                        delete[] pixels;

                        av_frame_unref(frame);

                        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / videoCodecContext_->framerate.num));
                    }
                    av_frame_free(&frame);
                }
            }
            av_packet_unref(packet);
        } else {
            isRunning_ = false;
        }
    }

    av_packet_free(&packet);
}
