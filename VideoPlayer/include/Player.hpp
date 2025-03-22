#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "AudioDecoder.hpp"
#include "MediaFile.hpp"
#include "VideoDecoder.hpp"

#include <thread>
#include <atomic>

class Player {
 public:
    Player();
    void Load(const std::string& filename);
    void Play();
    void TogglePause();
    void Stop();
    void Draw(sf::RenderWindow& window);
    void Seek(int seconds);
    void SetVolume(float volume);
    double GetDuration() const;
    double GetCurrentTime() const;
    float GetVolume() const;

 private:
    void PlaybackLoop();

    MediaFile mediaFile_;
    std::unique_ptr<VideoDecoder> videoDecoder_;
    std::unique_ptr<AudioDecoder> audioDecoder_;
    std::chrono::steady_clock::time_point startTime_;
    double timeOffset_;
    std::thread playbackThread_;
    std::atomic<bool> isRunning_;
    std::atomic<bool> isPaused_;
};

#endif
