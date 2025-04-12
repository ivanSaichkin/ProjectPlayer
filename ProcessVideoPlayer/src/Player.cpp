#include "../include/Player.hpp"

#include <algorithm>
#include <iostream>

Player::Player()
    : isPlaying_(false),
      isPaused_(false),
      isStopping_(false),
      currentTime_(0.0),
      volume_(100.0f),
      isMuted_(false),
      savedVolume_(100.0f),
      timeOffset_(0.0) {
}

Player::~Player() {
    Unload();
}

bool Player::Load(const std::string& filename, std::function<void(float)> progressCallback) {
    // Останавливаем текущее воспроизведение
    Unload();

    // Открываем файл
    if (!decoder_.Open(filename)) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return false;
    }

    // Декодируем все содержимое файла
    if (!decoder_.DecodeAll(progressCallback)) {
        std::cerr << "Failed to decode file" << std::endl;
        decoder_.Close();
        return false;
    }

    // Устанавливаем начальные значения
    currentTime_ = 0.0;
    timeOffset_ = 0.0;

    // Устанавливаем громкость
    decoder_.GetAudioManager().SetVolume(volume_);

    // Инициализируем спрайт с первым кадром
    const Frame* firstFrame = decoder_.GetFrameManager().GetFrameAtIndex(0);
    if (firstFrame) {
        currentSprite_.setTexture(firstFrame->GetTexture());
    }

    return true;
}

void Player::Unload() {
    // Останавливаем воспроизведение
    Stop();

    // Закрываем декодер
    decoder_.Close();

    // Сбрасываем состояние
    currentTime_ = 0.0;
    timeOffset_ = 0.0;
}

bool Player::IsLoaded() const {
    return decoder_.IsOpen();
}

void Player::Play() {
    if (!IsLoaded() || isPlaying_) {
        return;
    }

    // Если воспроизведение было на паузе, возобновляем его
    if (isPaused_) {
        isPaused_ = false;
        decoder_.GetAudioManager().Play();

        // Обновляем время начала с учетом паузы
        startTime_ = std::chrono::steady_clock::now() -
                     std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(currentTime_.load()));
        return;
    }

    // Запускаем воспроизведение с текущей позиции
    double seekTime = currentTime_.load();
    decoder_.GetAudioManager().SetCurrentTime(seekTime);
    decoder_.GetAudioManager().Play();

    // Запускаем поток воспроизведения
    isPlaying_ = true;
    isPaused_ = false;
    isStopping_ = false;
    startTime_ =
        std::chrono::steady_clock::now() - std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(seekTime));

    // Запускаем поток воспроизведения
    playbackThread_ = std::thread(&Player::PlaybackLoop, this);
    playbackThread_.detach();
}

void Player::Pause() {
    if (!isPlaying_ || isPaused_) {
        return;
    }

    isPaused_ = true;
    decoder_.GetAudioManager().Pause();
}

void Player::Stop() {
    if (!isPlaying_) {
        return;
    }

    // Сигнализируем потоку воспроизведения о необходимости остановки
    isStopping_ = true;

    // Останавливаем аудио
    decoder_.GetAudioManager().Stop();

    // Ждем, пока поток завершится
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Сбрасываем флаги
    isPlaying_ = false;
    isPaused_ = false;
    isStopping_ = false;

    // Сбрасываем время
    currentTime_ = 0.0;
    timeOffset_ = 0.0;

    // Отображаем первый кадр
    const Frame* firstFrame = decoder_.GetFrameManager().GetFrameAtIndex(0);
    if (firstFrame) {
        currentSprite_.setTexture(firstFrame->GetTexture());
    }
}

void Player::TogglePause() {
    if (isPaused_) {
        Play();
    } else {
        Pause();
    }
}

void Player::Seek(double time) {
    if (!IsLoaded()) {
        return;
    }

    // Ограничиваем время границами видео
    double targetTime = std::clamp(time, 0.0, GetDuration());

    // Обновляем текущее время
    currentTime_ = targetTime;

    // Если воспроизведение активно, обновляем аудио и время начала
    if (isPlaying_) {
        decoder_.GetAudioManager().SetCurrentTime(targetTime);

        // Если не на паузе, продолжаем воспроизведение
        if (!isPaused_) {
            decoder_.GetAudioManager().Play();
        }

        // Обновляем время начала
        startTime_ = std::chrono::steady_clock::now() -
                     std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(targetTime));
    }

    // Отображаем соответствующий кадр
    const Frame* frame = decoder_.GetFrameManager().GetFrameAtTime(targetTime);
    if (frame) {
        currentSprite_.setTexture(frame->GetTexture());
    }
}

double Player::GetDuration() const {
    return decoder_.GetDuration();
}

double Player::GetCurrentTime() const {
    return currentTime_.load();
}

bool Player::IsPlaying() const {
    return isPlaying_;
}

bool Player::IsPaused() const {
    return isPaused_;
}

void Player::SetVolume(float volume) {
    volume_ = std::clamp(volume, 0.0f, 100.0f);

    if (!isMuted_) {
        decoder_.GetAudioManager().SetVolume(volume_);
    }

    savedVolume_ = volume_;
}

float Player::GetVolume() const {
    return volume_;
}

void Player::Mute(bool mute) {
    isMuted_ = mute;

    if (mute) {
        decoder_.GetAudioManager().SetVolume(0.0f);
    } else {
        decoder_.GetAudioManager().SetVolume(volume_);
    }
}

bool Player::IsMuted() const {
    return isMuted_;
}

int Player::GetWidth() const {
    return decoder_.GetWidth();
}

int Player::GetHeight() const {
    return decoder_.GetHeight();
}

int Player::GetFrameRate() const {
    return decoder_.GetFrameRate();
}

void Player::Draw(Window& window) {
    // Масштабируем спрайт под размер окна
    window.ScaleSprite(currentSprite_, true);

    // Отрисовываем текущий кадр
    window.GetRenderWindow().draw(currentSprite_);
}

void Player::PlaybackLoop() {
    // Интервал обновления (примерно 60 FPS)
    const std::chrono::milliseconds updateInterval(16);

    while (isPlaying_ && !isStopping_) {
        // Если на паузе, просто ждем
        if (isPaused_) {
            std::this_thread::sleep_for(updateInterval);
            continue;
        }

        // Вычисляем текущее время
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - startTime_).count();

        // Ограничиваем время длительностью видео
        double duration = GetDuration();
        if (elapsed >= duration) {
            // Достигли конца видео
            currentTime_ = duration;

            // Останавливаем аудио
            decoder_.GetAudioManager().Stop();

            // Отображаем последний кадр
            const Frame* lastFrame = decoder_.GetFrameManager().GetFrameAtIndex(decoder_.GetFrameManager().GetFrameCount() - 1);

            if (lastFrame) {
                currentSprite_.setTexture(lastFrame->GetTexture());
            }

            // Завершаем воспроизведение
            isPlaying_ = false;
            isPaused_ = false;
            break;
        }

        // Обновляем текущее время
        currentTime_ = elapsed;

        // Получаем кадр для текущего времени
        const Frame* frame = decoder_.GetFrameManager().GetFrameAtTime(elapsed);
        if (frame) {
            currentSprite_.setTexture(frame->GetTexture());
        }

        // Ждем до следующего обновления
        std::this_thread::sleep_for(updateInterval);
    }

    isPlaying_ = false;
}
