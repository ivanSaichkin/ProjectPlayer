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

    void initialize();
    void start();
    void stop();

    // Render current frame to the target
    void render(sf::RenderTarget& target);

    // Get current playback position in seconds
    double getCurrentPosition() const;

    // Pause/resume rendering
    void setPaused(bool paused);
    bool isPaused() const;

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

    private:
        AudioDecoder& audioDecoder;
        std::vector<sf::Int16> buffer;

        // SoundStream overrides
        bool onGetData(Chunk& data) override;
        void onSeek(sf::Time timeOffset) override;
    };

    std::unique_ptr<CustomAudioStream> audioStream;
    std::thread renderThread;
    std::atomic<bool> running;
    std::atomic<bool> paused;
    std::atomic<double> currentPosition;

    // Rendering thread function
    void renderingLoop();
};
