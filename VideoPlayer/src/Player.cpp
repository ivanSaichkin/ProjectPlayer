#include "../include/Player.hpp"

#include <iostream>
#include <thread>

Player::Player() : timeOffset_(0.0), isRunning_(false) {
}

void Player::Load(const std::string& filename) {
    try {
        // Stop current playback if any
        Stop();

        // Load media file
        mediaFile_ = MediaFile();
        if (!mediaFile_.Load(filename)) {
            std::cerr << "Failed to load media file: " << filename << std::endl;
            return;
        }

        // Create decoders
        if (mediaFile_.GetVideoStreamIndex() >= 0) {
            videoDecoder_ = std::make_unique<VideoDecoder>(mediaFile_);
        }

        if (mediaFile_.GetAudioStreamIndex() >= 0) {
            audioDecoder_ = std::make_unique<AudioDecoder>(mediaFile_);
        }

        timeOffset_ = 0.0;

    } catch (const std::exception& e) {
        std::cerr << "Error loading media: " << e.what() << std::endl;
        videoDecoder_.reset();
        audioDecoder_.reset();
    }
}

void Player::Play() {
    if (!videoDecoder_ && !audioDecoder_) {
        std::cerr << "No media loaded" << std::endl;
        return;
    }

    timeOffset_ = 0.0;

    startTime_ = std::chrono::steady_clock::now();

    if (videoDecoder_) {
        videoDecoder_->SetStartTime(startTime_);
    }

    if (audioDecoder_) {
        audioDecoder_->SetStartTime(startTime_);
    }

    // Start the demuxing thread that will feed both decoders
    isRunning_ = true;
    isPaused_ = false;
    playbackThread_ = std::thread(&Player::PlaybackLoop, this);
    playbackThread_.detach();
}

void Player::PlaybackLoop() {
    AVPacket* packet = av_packet_alloc();
    if (!packet) {
        std::cerr << "Не удалось выделить память для пакета" << std::endl;
        isRunning_ = false;
        return;
    }

    // Запускаем потоки декодеров
    if (videoDecoder_) {
        videoDecoder_->Start();
    }

    if (audioDecoder_) {
        audioDecoder_->Start();
    }

    AVFormatContext* formatContext = mediaFile_.GetFormatContext();
    int videoStreamIndex = mediaFile_.GetVideoStreamIndex();
    int audioStreamIndex = mediaFile_.GetAudioStreamIndex();

    // Читаем пакеты и отправляем их соответствующим декодерам
    while (isRunning_) {
        // Обрабатываем состояние паузы
        while (isPaused_ && isRunning_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        if (!isRunning_) {
            break;
        }

        // Читаем следующий пакет
        int readResult = av_read_frame(formatContext, packet);

        if (readResult < 0) {
            // Если достигнут конец файла или произошла ошибка
            if (readResult == AVERROR_EOF) {

                // Сигнализируем декодерам о конце потока
                if (videoDecoder_) {
                    videoDecoder_->SignalEndOfStream();
                }

                if (audioDecoder_) {
                    audioDecoder_->SignalEndOfStream();
                }

                // Ждем, пока декодеры обработают все оставшиеся данные
                std::this_thread::sleep_for(std::chrono::milliseconds(500));

                // Если включено зацикливание, перемещаемся в начало файла
                // if (isLooping_) {
                //     Seek(-GetCurrentTime()); // Перемотка на начало
                //     continue;
                // }

                break;
            } else {
                char errBuf[AV_ERROR_MAX_STRING_SIZE];
                av_strerror(readResult, errBuf, sizeof(errBuf));
                std::cerr << "error reading packet: " << errBuf << std::endl;

                // Пытаемся продолжить чтение при некоторых ошибках
                if (readResult == AVERROR(EAGAIN) || readResult == AVERROR(EINTR)) {
                    continue;
                }

                break;
            }
        }

        // Направляем пакет соответствующему декодеру
        if (packet->stream_index == videoStreamIndex && videoDecoder_) {
            videoDecoder_->ProcessPacket(packet);
        } else if (packet->stream_index == audioStreamIndex && audioDecoder_) {
            audioDecoder_->ProcessPacket(packet);
        }

        // Освобождаем пакет для повторного использования
        av_packet_unref(packet);
    }

    // Освобождаем ресурсы
    av_packet_free(&packet);
    isRunning_ = false;

    std::cout << "playback end" << std::endl;
}

void Player::TogglePause() {
    isPaused_ = !isPaused_;

    if (videoDecoder_) {
        videoDecoder_->SetPaused(isPaused_);
    }

    if (audioDecoder_) {
        audioDecoder_->SetPaused(isPaused_);
    }

    // Update time offset when pausing/resuming
    timeOffset_ = GetCurrentTime();
    startTime_ = std::chrono::steady_clock::now() -
                 std::chrono::steady_clock::duration(
                     std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(timeOffset_)));
}

void Player::Stop() {
    isRunning_ = false;

    if (videoDecoder_) {
        videoDecoder_->Stop();
    }

    if (audioDecoder_) {
        audioDecoder_->Stop();
    }

    timeOffset_ = 0.0;

    // Wait for playback thread to finish
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void Player::Draw(Window& window) {
    if (videoDecoder_) {
        sf::Sprite& sprite = videoDecoder_->GetSprite();

        // Масштабируем спрайт под размер окна с сохранением пропорций
        window.ScaleSprite(sprite, true);

        // Отрисовываем видеокадр
        window.GetRenderWindow().draw(sprite);
    }
}

void Player::Seek(int seconds) {
    if (!mediaFile_.GetFormatContext()) {
        return;
    }

    // Временно останавливаем воспроизведение и декодирование
    bool wasPaused = isPaused_;
    isPaused_ = true;

    // Даем небольшую паузу, чтобы потоки декодирования могли остановиться
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Рассчитываем целевую позицию
    double currentTime = GetCurrentTime();
    double targetTime = currentTime + seconds;

    if (targetTime < 0) {
        targetTime = 0;
    } else if (targetTime > GetDuration()) {
        targetTime = GetDuration() - 0.5;  // Небольшой отступ от конца
    }

    // Конвертируем целевое время в timestamp для FFmpeg
    // Используем временную базу первого потока для поиска
    int64_t timestamp;
    AVRational timeBase;

    // Предпочитаем использовать видеопоток для поиска, если он доступен
    if (mediaFile_.GetVideoStreamIndex() >= 0) {
        AVStream* stream = mediaFile_.GetFormatContext()->streams[mediaFile_.GetVideoStreamIndex()];
        timeBase = stream->time_base;
        timestamp = static_cast<int64_t>(targetTime / av_q2d(timeBase));
    }
    // Иначе используем аудиопоток
    else if (mediaFile_.GetAudioStreamIndex() >= 0) {
        AVStream* stream = mediaFile_.GetFormatContext()->streams[mediaFile_.GetAudioStreamIndex()];
        timeBase = stream->time_base;
        timestamp = static_cast<int64_t>(targetTime / av_q2d(timeBase));
    }
    // Если ни один поток не найден, используем AV_TIME_BASE
    else {
        timestamp = static_cast<int64_t>(targetTime * AV_TIME_BASE);
        timeBase = {1, AV_TIME_BASE};
    }

    // Выполняем поиск в файле
    // Флаг AVSEEK_FLAG_BACKWARD заставляет искать ближайший предыдущий ключевой кадр
    int seekFlags = seconds < 0 ? AVSEEK_FLAG_BACKWARD : 0;
    int result = av_seek_frame(mediaFile_.GetFormatContext(), -1, timestamp, seekFlags);

    if (result < 0) {
        char errBuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(result, errBuf, sizeof(errBuf));
        std::cerr << "Ошибка при перемотке: " << errBuf << std::endl;

        // Восстанавливаем предыдущее состояние воспроизведения
        isPaused_ = wasPaused;
        return;
    }

    // Очищаем буферы декодеров
    if (videoDecoder_) {
        videoDecoder_->Flush();
    }

    if (audioDecoder_) {
        audioDecoder_->Flush();
    }

    // Обновляем временные метки
    timeOffset_ = targetTime;
    startTime_ = std::chrono::steady_clock::now() -
                 std::chrono::steady_clock::duration(
                     std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(timeOffset_)));

    if (videoDecoder_) {
        videoDecoder_->SetStartTime(startTime_);
    }

    if (audioDecoder_) {
        audioDecoder_->SetStartTime(startTime_);
    }

    // Восстанавливаем предыдущее состояние воспроизведения
    isPaused_ = wasPaused;

    std::cout << "Перемотка выполнена на позицию " << targetTime << " секунд" << std::endl;
}

void Player::SetVolume(float volume) {
    if (audioDecoder_) {
        audioDecoder_->SetVolume(volume);
    }
}

double Player::GetDuration() const {
    if (mediaFile_.GetFormatContext()) {
        return mediaFile_.GetFormatContext()->duration / (double)AV_TIME_BASE;
    }
    return 0.0;
}

double Player::GetCurrentTime() const {
    auto now = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(now - startTime_).count();
    return elapsed;
}

float Player::GetVolume() const {
    if (audioDecoder_) {
        return audioDecoder_->GetVolume();
    }
    return 0.0f;
}

sf::Vector2i Player::GetVideoSize() const {
    if (videoDecoder_) {
        return videoDecoder_->GetSize();
    }
    return sf::Vector2i(0, 0);
}
