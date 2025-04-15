#pragma once

#include "VideoDecoder.hpp"
#include "AudioDecoder.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <atomic>
#include <thread>

class MediaRenderer {
public:
    MediaRenderer(VideoDecoder& videoDecoder, AudioDecoder& audioDecoder);
    ~MediaRenderer();

    bool initialize();
    void start();
    void stop();

    // Render current frame to the target
    void render(sf::RenderTarget& target);

    // Get current playback position in seconds
    double getCurrentPosition() const;

    // Pause/resume rendering
    void setPaused(bool paused);
    bool isPaused() const;

    // Set volume (0.0 to 1.0)
    void setVolume(float volume);
    float getVolume() const;

private:
    VideoDecoder& videoDecoder;
    AudioDecoder& audioDecoder;

    sf::Sprite videoSprite;
    sf::Texture currentTexture;

    class CustomAudioStream : public sf::SoundStream {
    public:
        CustomAudioStream(AudioDecoder& decoder);
        void start();
        void stop();

        // Set volume (0.0 to 100.0 as per SFML)
        void setVolumeLevel(float volume);
        float getVolumeLevel() const;

    private:
        AudioDecoder& audioDecoder;
        std::vector<sf::Int16> buffer;
        float volumeLevel;

        // SoundStream overrides
        bool onGetData(Chunk& data) override;
        void onSeek(sf::Time timeOffset) override;
    };

    std::unique_ptr<CustomAudioStream> audioStream;
    std::thread renderThread;
    std::atomic<bool> running;
    std::atomic<bool> paused;
    std::atomic<double> currentPosition;
    std::atomic<float> volume;

    // Rendering thread function
    void renderingLoop();
};
