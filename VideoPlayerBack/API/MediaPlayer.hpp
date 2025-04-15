#pragma once

#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>

#include "../include/AudioDecoder.hpp"
#include "../include/VideoDecoder.hpp"

class MediaPlayer {
 public:
    // Constructor/Destructor
    MediaPlayer();
    ~MediaPlayer();

    // Media control methods
    bool open(const std::string& filename);
    void close();
    void play();
    void pause();
    void togglePlayPause();
    void seek(double seconds);
    void setVolume(float volume);
    float getVolume() const;

    // Status methods
    bool isPlaying() const;
    double getDuration() const;
    double getCurrentPosition() const;
    sf::Vector2u getVideoSize() const;
    double getFrameRate() const;
    unsigned int getAudioSampleRate() const;
    unsigned int getAudioChannelCount() const;

    // Frame access methods
    bool getCurrentFrame(sf::Texture& texture);
    void update();

    // Event callbacks
    void setPlaybackStartCallback(std::function<void()> callback);
    void setPlaybackPauseCallback(std::function<void()> callback);
    void setPlaybackStopCallback(std::function<void()> callback);
    void setPositionChangeCallback(std::function<void(double)> callback);
    void setFrameReadyCallback(std::function<void()> callback);
    void setErrorCallback(std::function<void(const MediaPlayerException&)> callback);

 private:
    // Decoders
    VideoDecoder videoDecoder;
    AudioDecoder audioDecoder;

    // Audio playback
    class CustomAudioStream : public sf::SoundStream {
     public:
        CustomAudioStream(AudioDecoder& decoder);
        void start();
        void stop();

     private:
        AudioDecoder& audioDecoder;
        std::vector<sf::Int16> buffer;

        bool onGetData(Chunk& data) override;
        void onSeek(sf::Time timeOffset) override;
    };

    std::unique_ptr<CustomAudioStream> audioStream;

    // Playback state
    std::atomic<bool> playing;
    std::atomic<float> volume;
    std::atomic<double> currentPosition;
    sf::Clock positionClock;

    // Current frame
    VideoFrame currentFrame;
    bool newFrameAvailable;
    std::mutex frameMutex;

    // Callbacks
    std::function<void()> playbackStartCallback;
    std::function<void()> playbackPauseCallback;
    std::function<void()> playbackStopCallback;
    std::function<void(double)> positionChangeCallback;
    std::function<void()> frameReadyCallback;

    // Internal methods
    void updatePosition();
    void notifyPositionChange();
};
