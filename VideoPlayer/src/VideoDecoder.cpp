#include "../include/VideoDecoder.hpp"

#include <iostream>

VideoDecoder::VideoDecoder(const MediaFile& mediaFile)
    : videoCodecContext_(nullptr), videoFrame_(nullptr), swsContext_(nullptr), mediaFile_(mediaFile) {
    isRunning_ = false;
    isPaused_ = false;
    endOfStream_ = false;
    hasFrame_ = false;
    preloadMode_ = false;

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
    // Очищаем буферы кодека
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

    // ИСПРАВЛЕНИЕ: Сохраняем флаг hasFrame_, чтобы избежать черного экрана при перемотке
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
    std::lock_guard<std::mutex> lock(mutex_);

    // Если нет контекста для преобразования, создаем его
    if (!swsContext_) {
        swsContext_ = sws_getContext(
            frame->width, frame->height, (AVPixelFormat)frame->format,
            frame->width, frame->height, AV_PIX_FMT_RGBA,
            SWS_BILINEAR, nullptr, nullptr, nullptr
        );

        if (!swsContext_) {
            std::cerr << "Не удалось создать контекст для преобразования изображения" << std::endl;
            return;
        }
    }

    // Вычисляем время отображения кадра
    double pts = frame->pts * mediaFile_.GetVideoTimeBase();

    // В режиме предварительной загрузки не проверяем время
    if (!preloadMode_) {
        // Вычисляем текущее время воспроизведения
        auto now = std::chrono::steady_clock::now();
        double currentTime = std::chrono::duration<double>(now - startTime_).count();

        // ИСПРАВЛЕНИЕ: Изменяем логику пропуска кадров
        // Если кадр еще не должен быть показан, и это не единственный кадр, выходим
        if (pts > currentTime + 0.5 && hasFrame_) {
            return;
        }

        // Если кадр уже должен был быть показан давно, и у нас есть более новые кадры в очереди,
        // пропускаем его, но только если это не единственный кадр
        if (pts < currentTime - 1.0 && hasFrame_ && !packetQueue_.empty()) {
            return;
        }
    }

    // Преобразуем кадр в RGBA формат
    uint8_t* data[4] = {nullptr};
    int linesize[4] = {0};

    // Вычисляем размер буфера для RGBA данных
    int bufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGBA, frame->width, frame->height, 1);
    std::vector<uint8_t> buffer(bufferSize);

    // Настраиваем указатели на буфер
    av_image_fill_arrays(data, linesize, buffer.data(), AV_PIX_FMT_RGBA, frame->width, frame->height, 1);

    // Выполняем преобразование
    sws_scale(swsContext_, frame->data, frame->linesize, 0, frame->height, data, linesize);

    // Обновляем текстуру
    texture_.update(buffer.data());

    // Устанавливаем флаг наличия кадра
    hasFrame_ = true;
}

void VideoDecoder::DecodeVideo() {
    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        std::cerr << "Не удалось выделить память для кадра" << std::endl;
        return;
    }

    // Флаг, указывающий, получили ли мы хотя бы один кадр
    bool receivedFirstFrame = false;

    while (isRunning_) {
        // Обрабатываем состояние паузы
        if (isPaused_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        if (!isRunning_) {
            break;
        }

        // Получаем пакет из очереди
        AVPacket* packet = nullptr;
        bool gotPacket = false;

        {
            std::unique_lock<std::mutex> lock(queueMutex_);

            // ИСПРАВЛЕНИЕ: Уменьшаем время ожидания для более плавного воспроизведения
            auto waitResult = packetCondition_.wait_for(lock, std::chrono::milliseconds(50), [this] {
                return !packetQueue_.empty() || endOfStream_ || !isRunning_;
            });

            if (!isRunning_) {
                break;
            }

            if (!packetQueue_.empty()) {
                packet = packetQueue_.front();
                packetQueue_.pop();
                gotPacket = true;
            }
        }

        // Обрабатываем пакет
        if (gotPacket) {
            int response = avcodec_send_packet(videoCodecContext_, packet);
            av_packet_free(&packet);

            if (response < 0) {
                std::cerr << "Ошибка при отправке пакета для декодирования" << std::endl;
                continue;
            }

            // Получаем кадры
            bool gotFrame = false;
            while (response >= 0) {
                response = avcodec_receive_frame(videoCodecContext_, frame);

                if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
                    break;
                } else if (response < 0) {
                    std::cerr << "Ошибка при декодировании видео" << std::endl;
                    break;
                }

                // Обрабатываем кадр
                ProcessVideoFrame(frame);
                gotFrame = true;
                receivedFirstFrame = true;
            }
        }
        else if (endOfStream_) {
            // Очищаем декодер
            avcodec_send_packet(videoCodecContext_, nullptr);

            while (true) {
                int response = avcodec_receive_frame(videoCodecContext_, frame);
                if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
                    break;
                } else if (response < 0) {
                    std::cerr << "Ошибка при очистке декодера" << std::endl;
                    break;
                }

                // Обрабатываем кадр
                ProcessVideoFrame(frame);
            }

            // Выходим из цикла, когда все кадры обработаны
            if (packetQueue_.empty()) {
                break;
            }
        }
        else {
            // Если очередь пуста, даем процессору отдохнуть
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
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

bool VideoDecoder::HasFrame() const {
    return hasFrame_;
}

void VideoDecoder::SetPreloadMode(bool preload) {
    preloadMode_ = preload;
}
