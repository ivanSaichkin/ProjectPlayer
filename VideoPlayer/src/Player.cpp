#include "../include/Player.hpp"

#include <algorithm>
#include <iostream>

Player::Player() : timeOffset_(0.0), isRunning_(false), isPaused_(false), isFinished_(false), isSeeking_(false) {
}

Player::~Player() {
    Stop();
}

void Player::Load(const std::string& filename) {
    try {
        // Останавливаем текущее воспроизведение
        Stop();

        // Загружаем медиафайл
        mediaFile_ = MediaFile();
        if (!mediaFile_.Load(filename)) {
            std::cerr << "Не удалось загрузить медиафайл: " << filename << std::endl;
            return;
        }

        // Создаем декодеры
        if (mediaFile_.GetVideoStreamIndex() >= 0) {
            videoDecoder_ = std::make_unique<VideoDecoder>(mediaFile_);
        }

        if (mediaFile_.GetAudioStreamIndex() >= 0) {
            audioDecoder_ = std::make_unique<AudioDecoder>(mediaFile_);
        }

        // ИСПРАВЛЕНИЕ: Явно сбрасываем все переменные времени и флаги
        timeOffset_ = 0.0;
        isFinished_ = false;
        isPaused_ = false;
        isSeeking_ = false;

        // Предварительно загружаем первый кадр
        PreloadFirstFrame();

    } catch (const std::exception& e) {
        std::cerr << "Ошибка при загрузке медиафайла: " << e.what() << std::endl;
        videoDecoder_.reset();
        audioDecoder_.reset();
    }
}

void Player::PreloadFirstFrame() {
    if (!videoDecoder_)
        return;

    // Запускаем декодер видео для получения первого кадра
    videoDecoder_->Start();

    // Читаем и отправляем пакеты до получения первого кадра
    bool frameReceived = false;
    int maxPacketsToRead = 100;  // ИСПРАВЛЕНИЕ: Увеличиваем количество попыток

    AVPacket* packet = av_packet_alloc();
    if (!packet) {
        std::cerr << "Не удалось выделить память для пакета" << std::endl;
        return;
    }

    AVFormatContext* formatContext = mediaFile_.GetFormatContext();
    int videoStreamIndex = mediaFile_.GetVideoStreamIndex();

    videoDecoder_->SetPreloadMode(true);

    for (int i = 0; i < maxPacketsToRead && !frameReceived; i++) {
        int readResult = av_read_frame(formatContext, packet);

        if (readResult < 0) {
            break;
        }

        if (packet->stream_index == videoStreamIndex) {
            videoDecoder_->ProcessPacket(packet);
            frameReceived = videoDecoder_->HasFrame();

            // ИСПРАВЛЕНИЕ: Добавляем паузу для обработки кадра
            if (i % 10 == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }

        av_packet_unref(packet);
    }

    videoDecoder_->SetPreloadMode(false);
    av_packet_free(&packet);

    // Сбрасываем позицию файла в начало
    av_seek_frame(formatContext, -1, 0, AVSEEK_FLAG_BACKWARD);

    // Останавливаем декодер после загрузки первого кадра
    videoDecoder_->Stop();

    std::cout << "Предварительная загрузка " << (frameReceived ? "успешна" : "не удалась") << std::endl;
}

void Player::Play() {
    if (!videoDecoder_ && !audioDecoder_) {
        std::cerr << "Нет загруженных медиаданных" << std::endl;
        return;
    }

    // Если воспроизведение уже запущено, просто снимаем с паузы
    if (isRunning_) {
        isPaused_ = false;
        if (videoDecoder_) {
            videoDecoder_->SetPaused(false);
        }
        if (audioDecoder_) {
            audioDecoder_->SetPaused(false);
        }
        return;
    }

    // Сбрасываем флаг завершения и флаг поиска
    isFinished_ = false;
    isSeeking_ = false;

    // ИСПРАВЛЕНИЕ: Правильное преобразование времени
    startTime_ = std::chrono::steady_clock::now() -
             std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                 std::chrono::duration<double>(timeOffset_));

    if (videoDecoder_) {
        videoDecoder_->SetStartTime(startTime_);
    }

    if (audioDecoder_) {
        audioDecoder_->SetStartTime(startTime_);
    }

    // Запускаем поток воспроизведения
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

        // Проверяем, не находимся ли мы в процессе перемотки
        if (isSeeking_) {
            std::unique_lock<std::mutex> lock(seekMutex_);
            seekCondition_.wait(lock, [this] { return !isSeeking_ || !isRunning_; });

            if (!isRunning_) {
                break;
            }
        }

        if (!isRunning_) {
            break;
        }

        // Читаем следующий пакет
        int readResult = av_read_frame(formatContext, packet);

        if (readResult < 0) {
            // Если достигнут конец файла или произошла ошибка
            if (readResult == AVERROR_EOF) {
                std::cout << "Достигнут конец файла" << std::endl;

                // Сигнализируем декодерам о конце потока
                if (videoDecoder_) {
                    videoDecoder_->SignalEndOfStream();
                }

                if (audioDecoder_) {
                    audioDecoder_->SignalEndOfStream();
                }

                // Устанавливаем флаг завершения
                isFinished_ = true;

                // Ждем, пока декодеры обработают все оставшиеся данные
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                break;
            } else {
                char errBuf[AV_ERROR_MAX_STRING_SIZE];
                av_strerror(readResult, errBuf, sizeof(errBuf));
                std::cerr << "Ошибка при чтении пакета: " << errBuf << std::endl;

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

    std::cout << "Воспроизведение завершено" << std::endl;
}

void Player::TogglePause() {
    isPaused_ = !isPaused_;

    if (videoDecoder_) {
        videoDecoder_->SetPaused(isPaused_);
    }

    if (audioDecoder_) {
        audioDecoder_->SetPaused(isPaused_);
    }

    if (isPaused_) {
        // При паузе сохраняем текущее смещение времени
        timeOffset_ = GetCurrentTime();
    } else {
        // При возобновлении обновляем время начала с учетом смещения
        startTime_ = std::chrono::steady_clock::now() -
                     std::chrono::steady_clock::duration(
                         std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(timeOffset_)));

        if (videoDecoder_) {
            videoDecoder_->SetStartTime(startTime_);
        }

        if (audioDecoder_) {
            audioDecoder_->SetStartTime(startTime_);
        }
    }

    std::cout << (isPaused_ ? "Пауза" : "Возобновление") << std::endl;
}

void Player::Stop() {
    isRunning_ = false;
    isPaused_ = false;
    isFinished_ = false;

    // Уведомляем о прекращении поиска, если он был активен
    {
        std::lock_guard<std::mutex> lock(seekMutex_);
        isSeeking_ = false;
    }
    seekCondition_.notify_all();

    if (videoDecoder_) {
        videoDecoder_->Stop();
    }

    if (audioDecoder_) {
        audioDecoder_->Stop();
    }

    timeOffset_ = 0.0;

    // Ждем завершения потока воспроизведения
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
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

    // ИСПРАВЛЕНИЕ: Останавливаем воспроизведение для надежной перемотки
    bool wasRunning = isRunning_;
    bool wasPaused = isPaused_;
    isRunning_ = false;

    // Даем время потокам завершиться
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Устанавливаем флаг перемотки
    {
        std::lock_guard<std::mutex> lock(seekMutex_);
        isSeeking_ = true;
    }

    // Рассчитываем целевую позицию
    double currentTime = GetCurrentTime();
    double targetTime = currentTime + seconds;

    if (targetTime < 0) {
        targetTime = 0;
    } else if (targetTime > GetDuration()) {
        targetTime = GetDuration() - 0.5;  // Небольшой отступ от конца
    }

    // Получаем временную базу для поиска
    int64_t timestamp;
    AVRational timeBase;
    int streamIndex = -1;

    // Предпочитаем использовать видеопоток для поиска
    if (mediaFile_.GetVideoStreamIndex() >= 0) {
        streamIndex = mediaFile_.GetVideoStreamIndex();
        AVStream* stream = mediaFile_.GetFormatContext()->streams[streamIndex];
        timeBase = stream->time_base;
        timestamp = static_cast<int64_t>(targetTime / av_q2d(timeBase));
    }
    // Иначе используем аудиопоток
    else if (mediaFile_.GetAudioStreamIndex() >= 0) {
        streamIndex = mediaFile_.GetAudioStreamIndex();
        AVStream* stream = mediaFile_.GetFormatContext()->streams[streamIndex];
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
    int seekFlags = AVSEEK_FLAG_BACKWARD;
    int result = av_seek_frame(mediaFile_.GetFormatContext(), streamIndex, timestamp, seekFlags);

    if (result < 0) {
        char errBuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(result, errBuf, sizeof(errBuf));
        std::cerr << "Ошибка при перемотке: " << errBuf << std::endl;

        // Восстанавливаем предыдущее состояние и выходим
        isRunning_ = wasRunning;
        isPaused_ = wasPaused;

        // Снимаем флаг перемотки
        {
            std::lock_guard<std::mutex> lock(seekMutex_);
            isSeeking_ = false;
        }
        seekCondition_.notify_all();
        return;
    }

    // Очищаем буферы декодеров
    if (videoDecoder_) {
        videoDecoder_->Flush();
    }

    if (audioDecoder_) {
        audioDecoder_->Flush();
    }

    // Сбрасываем флаг завершения
    isFinished_ = false;

    // Обновляем временные метки
    timeOffset_ = targetTime;

    // ИСПРАВЛЕНИЕ: Правильное преобразование времени
    startTime_ = std::chrono::steady_clock::now() -
             std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                 std::chrono::duration<double>(timeOffset_));

    // Перезапускаем воспроизведение если оно было активно
    if (wasRunning) {
        isRunning_ = true;
        isPaused_ = wasPaused;

        // Перезапускаем поток воспроизведения
        playbackThread_ = std::thread(&Player::PlaybackLoop, this);
        playbackThread_.detach();
    }

    // Обновляем время для декодеров
    if (videoDecoder_) {
        videoDecoder_->SetStartTime(startTime_);
    }

    if (audioDecoder_) {
        audioDecoder_->SetStartTime(startTime_);
    }

    // Снимаем флаг перемотки
    {
        std::lock_guard<std::mutex> lock(seekMutex_);
        isSeeking_ = false;
    }
    seekCondition_.notify_all();

    std::cout << "Перемотка выполнена на позицию " << targetTime << " секунд" << std::endl;
}

void Player::SetVolume(float volume) {
    if (audioDecoder_) {
        audioDecoder_->SetVolume(std::clamp(volume, 0.0f, 100.0f));
    }
}

double Player::GetDuration() const {
    if (!mediaFile_.GetFormatContext()) {
        return 0.0;
    }

    double duration = mediaFile_.GetFormatContext()->duration / (double)AV_TIME_BASE;
    return duration > 0 ? duration : 0.0;
}

double Player::GetCurrentTime() const {
    // ИСПРАВЛЕНИЕ: Добавляем проверку на isRunning_
    if (!isRunning_ && !isFinished_) {
        return timeOffset_;
    }

    if (isFinished_) {
        // Если воспроизведение завершено, возвращаем длительность видео
        return GetDuration();
    }

    if (isPaused_) {
        return timeOffset_;
    }

    auto now = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(now - startTime_).count();

    // Ограничиваем возвращаемое значение длительностью видео
    double duration = GetDuration();
    if (elapsed > duration) {
        return duration;
    }

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

bool Player::IsFinished() const {
    return isFinished_;
}

bool Player::IsPaused() const {
    return isPaused_;
}
