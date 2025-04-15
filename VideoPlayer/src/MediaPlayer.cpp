#include "../include/MediaPlayer.hpp"
#include <iostream>

MediaPlayer::MediaPlayer()
    : renderer(videoDecoder, audioDecoder),
      isPlaying(false) {
}

MediaPlayer::~MediaPlayer() {
    close();
}

bool MediaPlayer::open(const std::string& filename) {
    // Close current media if any
    close();

    // Open media file
    if (!videoDecoder.open(filename)) {
        std::cerr << "Failed to open video decoder" << std::endl;
        return false;
    }

    // Initialize video decoder
    if (!videoDecoder.initialize()) {
        std::cerr << "Failed to initialize video decoder" << std::endl;
        return false;
    }

    // Open audio decoder (same file)
    if (!audioDecoder.open(filename)) {
        std::cerr << "Failed to open audio decoder" << std::endl;
        // Continue without audio
    } else {
        // Initialize audio decoder
        if (!audioDecoder.initialize()) {
            std::cerr << "Failed to initialize audio decoder" << std::endl;
            // Continue without audio
        }
    }

    // Initialize renderer
    renderer.initialize();

    // Create window based on video size
    sf::Vector2u videoSize = videoDecoder.getSize();
    if (videoSize.x == 0 || videoSize.y == 0) {
        videoSize = sf::Vector2u(800, 600);  // Default size
    }

    window.create(sf::VideoMode(videoSize.x, videoSize.y), "SFML Video Player");
    window.setFramerateLimit(60);

    // Initialize controls
    controls.initialize(sf::Vector2f(videoSize.x, videoSize.y));
    controls.setPlayPauseCallback([this]() { togglePlayPause(); });
    controls.setSeekCallback([this](double seconds) { seek(seconds); });

    return true;
}

void MediaPlayer::close() {
    renderer.stop();
    window.close();
}

void MediaPlayer::run() {
    if (!window.isOpen()) {
        return;
    }

    // Start playback
    renderer.start();
    isPlaying = true;

    // Main loop
    while (window.isOpen()) {
        handleEvents();
        update();
        render();
    }
}

void MediaPlayer::play() {
    if (!isPlaying) {
        isPlaying = true;
        renderer.setPaused(false);
    }
}

void MediaPlayer::pause() {
    if (isPlaying) {
        isPlaying = false;
        renderer.setPaused(true);
    }
}

void MediaPlayer::togglePlayPause() {
    if (isPlaying) {
        pause();
    } else {
        play();
    }
}

void MediaPlayer::seek(double seconds) {
    videoDecoder.seek(seconds);
    audioDecoder.seek(seconds);
}

void MediaPlayer::handleEvents() {
    sf::Event event;
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            window.close();
        } else if (event.type == sf::Event::KeyPressed) {
            if (event.key.code == sf::Keyboard::Space) {
                togglePlayPause();
            } else if (event.key.code == sf::Keyboard::Left) {
                seek(std::max(0.0, renderer.getCurrentPosition() - 5.0));  // Seek 5 seconds back
            } else if (event.key.code == sf::Keyboard::Right) {
                seek(renderer.getCurrentPosition() + 5.0);  // Seek 5 seconds forward
            } else if (event.key.code == sf::Keyboard::Escape) {
                window.close();
            }
        }

        // Handle control events
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        controls.handleEvent(event, sf::Vector2f(mousePos.x, mousePos.y));
    }
}

void MediaPlayer::update() {
    // Update controls with current playback position
    double currentPosition = renderer.getCurrentPosition();
    double duration = videoDecoder.getDuration();
    controls.updateProgress(currentPosition, duration);
}

void MediaPlayer::render() {
    window.clear(sf::Color::Black);

    // Render video frame
    renderer.render(window);

    // Render controls
    controls.render(window);

    window.display();
}
