#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <atomic>
#include <functional>
#include <thread>

#include "Decoder.hpp"
#include "../../Window/include/Window.hpp"

class Player {
 public:
    Player();
    ~Player();

    // Загрузка и декодирование видео
    bool Load(const std::string& filename, std::function<void(float)> progressCallback = nullptr);
    void Unload();
    bool IsLoaded() const;

    // Управление воспроизведением
    void Play();
    void Pause();
    void Stop();
    void TogglePause();
    void Seek(double time);

    // Информация о воспроизведении
    double GetDuration() const;
    double GetCurrentTime() const;
    bool IsPlaying() const;
    bool IsPaused() const;

    // Управление аудио
    void SetVolume(float volume);
    float GetVolume() const;
    void Mute(bool mute);
    bool IsMuted() const;

    // Информация о видео
    int GetWidth() const;
    int GetHeight() const;
    int GetFrameRate() const;

    // Отрисовка текущего кадра
    void Draw(Window& window);

 private:
    // Основной цикл воспроизведения
    void PlaybackLoop();

    // Объекты для работы с видео и аудио
    Decoder decoder_;
    sf::Sprite currentSprite_;

    // Переменные состояния
    std::atomic<bool> isPlaying_;
    std::atomic<bool> isPaused_;
    std::atomic<bool> isStopping_;
    std::atomic<double> currentTime_;
    std::atomic<float> volume_;
    std::atomic<bool> isMuted_;
    float savedVolume_;

    // Поток воспроизведения
    std::thread playbackThread_;

    // Время начала воспроизведения
    std::chrono::steady_clock::time_point startTime_;
    double timeOffset_;
};

#endif
