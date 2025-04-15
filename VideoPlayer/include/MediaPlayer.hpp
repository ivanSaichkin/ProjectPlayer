#pragma once

#include "VideoDecoder.hpp"
#include "AudioDecoder.hpp"
#include "MediaRenderer.hpp"
#include "PlayerControls.hpp"
#include <SFML/Graphics.hpp>
#include <string>

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

private:
    VideoDecoder videoDecoder;
    AudioDecoder audioDecoder;
    MediaRenderer renderer;
    PlayerControls controls;

    sf::RenderWindow window;
    bool isPlaying;

    // Handle window events
    void handleEvents();

    // Update player state
    void update();

    // Render the player
    void render();
};
