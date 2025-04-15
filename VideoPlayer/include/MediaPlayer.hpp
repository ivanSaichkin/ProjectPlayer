#pragma once

#include <SFML/Graphics.hpp>
#include <string>

#include "AudioDecoder.hpp"
#include "ErrorHandler.hpp"
#include "MediaRenderer.hpp"
#include "PlayerControls.hpp"
#include "VideoDecoder.hpp"

class MediaPlayer {
 public:
    MediaPlayer();
    ~MediaPlayer();

    // Open a media file
    bool open(const std::string& filename);

    // Close the current media
    void close();

    // Run the player (main loop)
    void run();

    // Control playback
    void play();
    void pause();
    void togglePlayPause();
    void seek(double seconds);

    // Volume control
    void setVolume(float volume);
    float getVolume() const;

    // Error handling
    void setErrorCallback(std::function<void(const MediaPlayerException&)> callback);

 private:
    VideoDecoder videoDecoder;
    AudioDecoder audioDecoder;
    MediaRenderer renderer;
    PlayerControls controls;

    sf::RenderWindow window;
    bool isPlaying;
    bool isFullscreen;
    sf::Vector2u originalSize;

    // Handle window events
    void handleEvents();

    // Update player state
    void update();

    // Render the player
    void render();

    // Toggle fullscreen mode
    void toggleFullscreen();

    // Default error handler
    void onError(const MediaPlayerException& error);
};
