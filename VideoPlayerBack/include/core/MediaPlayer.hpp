#pragma once

#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "AudioDecoder.hpp"
#include "MediaPlayerException.hpp"
#include "VideoDecoder.hpp"

namespace VideoPlayer {
namespace Core {

class MediaPlayer {
 public:
    // Constructor and destructor
    MediaPlayer();
    ~MediaPlayer();

    // File operations
    bool open(const std::string& filePath);
    void close();
    bool isOpen() const;

    // Playback control
    void play();
    void pause();
    void stop();
    void seek(double position);

    // Status information
    bool isPlaying() const;
    bool isPaused() const;
    double getCurrentPosition() const;
    double getDuration() const;
    sf::Vector2i getVideoSize() const;

    // Audio control
    void setVolume(float volume);
    float getVolume() const;

    // Video frame access
    const sf::Texture& getCurrentFrame() const;

    // Event callbacks
    void setPlaybackStartCallback(std::function<void()> callback);
    void setPlaybackPauseCallback(std::function<void()> callback);
    void setPlaybackStopCallback(std::function<void()> callback);
    void setPlaybackEndCallback(std::function<void()> callback);
    void setPositionChangeCallback(std::function<void(double)> callback);
    void setFrameReadyCallback(std::function<void()> callback);
    void setErrorCallback(std::function<void(const MediaPlayerException&)> callback);

    // Update function (call this regularly from main thread)
    void update();

 private:
    // Media decoders
    std::unique_ptr<AudioDecoder> audioDecoder;
    std::unique_ptr<VideoDecoder> videoDecoder;

    // Playback state
    std::atomic<bool> playing;
    std::atomic<bool> paused;
    std::atomic<double> currentPosition;
    std::atomic<double> duration;
    std::atomic<float> volume;

    // Video rendering
    sf::Texture currentFrame;
    sf::Mutex frameMutex;
    sf::Clock frameClock;
    double frameTime;

    // Audio playback
    sf::SoundBuffer soundBuffer;
    sf::Sound sound;
    std::vector<sf::Int16> audioSamples;

    // Playback thread
    std::thread playbackThread;
    std::atomic<bool> threadRunning;

    // Callbacks
    std::function<void()> playbackStartCallback;
    std::function<void()> playbackPauseCallback;
    std::function<void()> playbackStopCallback;
    std::function<void()> playbackEndCallback;
    std::function<void(double)> positionChangeCallback;
    std::function<void()> frameReadyCallback;
    std::function<void(const MediaPlayerException&)> errorCallback;

    // Internal methods
    void playbackLoop();
    bool decodeNextVideoFrame();
    bool decodeNextAudioChunk();
    void updateCurrentPosition();
    void checkEndOfPlayback();
    void handleError(MediaPlayerException::ErrorCode code, const std::string& message);
};

}  // namespace Core
}  // namespace VideoPlayer
