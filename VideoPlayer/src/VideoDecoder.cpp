#include "../include/VideoDecoder.hpp"

VideoDecoder::VideoDecoder(const MediaFile& mediaFile) : mediaFile_(mediaFile), videoCodecContext_(nullptr), swsContext_(nullptr) {
    isPaused_ = false;
    isRunning_ = false;

    AVCodecParameters* codecParams = mediaFile_.GetFormatContext()->streams[mediaFile_.GetVideoStreamIndex()]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);

    if (!codec)
        throw VideoDecoderError("Unsupported video codec");

    videoCodecContext_ = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(videoCodecContext_, codecParams);

    if (avcodec_open2(videoCodecContext_, codec, nullptr) < 0)
        throw VideoDecoderError("Failed to open video codec");

    swsContext_ = sws_getContext(videoCodecContext_->width, videoCodecContext_->height, videoCodecContext_->pix_fmt, videoCodecContext_->width,
                                 videoCodecContext_->height, AV_PIX_FMT_RGB32, SWS_BILINEAR, nullptr, nullptr, nullptr);

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

void VideoDecoder::Draw(sf::RenderWindow& window) {
    window.draw(sprite_);
}

void VideoDecoder::DecodeVideo() {
    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    std::unique_lock<std::mutex> lock(mutex_, std::defer_lock);

    if (!packet || !frame) {
        throw VideoDecoderError("Failed to allocate FFmpeg structures");
    }

    while (isRunning_) {
        if (isPaused_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        int read_result = av_read_frame(mediaFile_.GetFormatContext(), packet);
        if (read_result < 0) {
            // Конец файла или ошибка чтения
            if (read_result == AVERROR_EOF) {
                isRunning_ = false;
            }
            continue;
        }

        if (packet->stream_index == mediaFile_.GetVideoStreamIndex()) {
            if (avcodec_send_packet(videoCodecContext_, packet) != 0) {
                av_packet_unref(packet);
                continue;
            }

            while (avcodec_receive_frame(videoCodecContext_, frame) == 0) {
                // Конвертация в RGB32 для SFML
                uint8_t* pixels = new uint8_t[videoCodecContext_->width * videoCodecContext_->height * 4];
                uint8_t* dest[4] = {pixels};
                int linesize[4] = {videoCodecContext_->width * 4};

                sws_scale(swsContext_, frame->data, frame->linesize, 0, videoCodecContext_->height, dest, linesize);

                // Блокировка мьютекса для обновления текстуры
                lock.lock();
                texture_.update(pixels);
                lock.unlock();

                delete[] pixels;

                // Синхронизация по времени
                if (frame->best_effort_timestamp != AV_NOPTS_VALUE) {
                    double pts = frame->best_effort_timestamp * mediaFile_.GetVideoTimeBase();
                    auto target_time = startTime_ + std::chrono::duration<double>(pts);
                    std::this_thread::sleep_until(target_time);
                }
            }
        }
        av_packet_unref(packet);
    }

    av_packet_free(&packet);
    av_frame_free(&frame);
}

void VideoDecoder::Flush() {
    avcodec_flush_buffers(videoCodecContext_);
}
