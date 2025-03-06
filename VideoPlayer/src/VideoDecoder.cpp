#include "../include/VideoDecoder.hpp"

VideoDecoder::VideoDecoder(const MediaFile& mediaFile) : mediaFile_(mediaFile) {
    AVCodecParameters* videoCodecParameters = mediaFile_.GetFormatContext()->streams[mediaFile_.GetVideoStreamIndex()]->codecpar;
    const AVCodec* videoCodec = avcodec_find_decoder(videoCodecParameters->codec_id);
    if (!videoCodec) {
        throw std::runtime_error("Unsupported video codec");
    }

    videoCodecContext_ = avcodec_alloc_context3(videoCodec);
    avcodec_parameters_to_context(videoCodecContext_, videoCodecParameters);

    if (avcodec_open2(videoCodecContext_, videoCodec, nullptr) < 0) {
        throw std::runtime_error("Failed to open video codec");
    }

    videoFrame_ = av_frame_alloc();
    swsContext_ = sws_getContext(videoCodecContext_->width, videoCodecContext_->height, videoCodecContext_->pix_fmt, videoCodecContext_->width,
                                 videoCodecContext_->height, AV_PIX_FMT_RGB32, SWS_BILINEAR, nullptr, nullptr, nullptr);
    texture_.create(videoCodecContext_->width, videoCodecContext_->height);
    sprite_.setTexture(texture_);
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

void VideoDecoder::DecodeVideo() {
    AVPacket* packet = av_packet_alloc();

    while (isRunning_) {
        if (av_read_frame(mediaFile_.GetFormatContext(), packet) >= 0) {
            if (packet->stream_index == mediaFile_.GetVideoStreamIndex()) {
                if (avcodec_send_packet(videoCodecContext_, packet) == 0) {
                    while (avcodec_receive_frame(videoCodecContext_, videoFrame_) == 0) {
                        uint8_t* pixels = new uint8_t[videoCodecContext_->width * videoCodecContext_->height * 4];
                        uint8_t* dest[4] = {pixels, nullptr, nullptr, nullptr};
                        int destLinesize[4] = {videoCodecContext_->width * 4, 0, 0, 0};

                        sws_scale(swsContext_, videoFrame_->data, videoFrame_->linesize, 0, videoCodecContext_->height, dest, destLinesize);

                        texture_.update(pixels);
                        delete[] pixels;

                        sprite_.setTexture(texture_);
                    }
                }
            }
            av_packet_unref(packet);
        } else {
            isRunning_ = false;
        }
    }
    av_packet_free(&packet);
}
