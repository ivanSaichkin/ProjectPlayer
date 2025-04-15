#include "../include/MediaPlayer.hpp"

#include <iostream>

MediaPlayer::MediaPlayer() : renderer(videoDecoder, audioDecoder), isPlaying(false), isFullscreen(false) {
    // Set default error callback
    ErrorHandler::getInstance().setErrorCallback([this](const MediaPlayerException& error) { onError(error); });
}

MediaPlayer::~MediaPlayer() {
    close();
}

bool MediaPlayer::open(const std::string& filename) {
    try {
        // Close current media if any
        close();

        // Open media file
        if (!videoDecoder.open(filename)) {
            ErrorHandler::getInstance().handleError(MediaPlayerException::FILE_NOT_FOUND, "Failed to open video file: " + filename);
            return false;
        }

        // Initialize video decoder
        if (!videoDecoder.initialize()) {
            ErrorHandler::getInstance().handleError(MediaPlayerException::DECODER_ERROR, "Failed to initialize video decoder for: " + filename);
            return false;
        }

        // Open audio decoder (same file)
        if (!audioDecoder.open(filename)) {
            std::cerr << "Warning: Failed to open audio decoder, continuing without audio" << std::endl;
            // Continue without audio
        } else {
            // Initialize audio decoder
            if (!audioDecoder.initialize()) {
                std::cerr << "Warning: Failed to initialize audio decoder, continuing without audio" << std::endl;
                // Continue without audio
            }
        }

        // Initialize renderer
        if (!renderer.initialize()) {
            ErrorHandler::getInstance().handleError(MediaPlayerException::RENDER_ERROR, "Failed to initialize media renderer");
            return false;
        }

        // Create window based on video size
        sf::Vector2u videoSize = videoDecoder.getSize();
        if (videoSize.x == 0 || videoSize.y == 0) {
            videoSize = sf::Vector2u(800, 600);  // Default size
        }

        originalSize = videoSize;
        window.create(sf::VideoMode(videoSize.x, videoSize.y), "SFML Video Player");
        window.setFramerateLimit(60);

        // Initialize controls
        controls.initialize(sf::Vector2f(videoSize.x, videoSize.y));
        controls.setPlayPauseCallback([this]() { togglePlayPause(); });
        controls.setSeekCallback([this](double seconds) { seek(seconds); });
        controls.setVolumeCallback([this](float vol) { setVolume(vol); });

        return true;
    } catch (const std::exception& e) {
        ErrorHandler::getInstance().handleError(MediaPlayerException::UNKNOWN_ERROR, std::string("Exception during media opening: ") + e.what());
        return false;
    }
}

void MediaPlayer::close() {
    try {
        renderer.stop();
        window.close();
    } catch (const std::exception& e) {
        ErrorHandler::getInstance().handleError(MediaPlayerException::UNKNOWN_ERROR, std::string("Exception during media closing: ") + e.what());
    }
}

void MediaPlayer::run() {
    if (!window.isOpen()) {
        return;
    }

    try {
        // Start playback
        renderer.start();
        isPlaying = true;

        // Main loop
        while (window.isOpen()) {
            handleEvents();
            update();
            render();
        }
    } catch (const std::exception& e) {
        ErrorHandler::getInstance().handleError(MediaPlayerException::UNKNOWN_ERROR, std::string("Exception during playback: ") + e.what());
    }
}

void MediaPlayer::play() {
    if (!isPlaying) {
        isPlaying = true;
        renderer.setPaused(false);
        controls.updateVolume(renderer.getVolume());
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
    try {
        videoDecoder.seek(seconds);
        audioDecoder.seek(seconds);
    } catch (const std::exception& e) {
        ErrorHandler::getInstance().handleError(MediaPlayerException::DECODER_ERROR, std::string("Exception during seek: ") + e.what());
    }
}

void MediaPlayer::setVolume(float vol) {
    try {
        renderer.setVolume(vol);
        controls.updateVolume(vol);
    } catch (const std::exception& e) {
        ErrorHandler::getInstance().handleError(MediaPlayerException::RENDER_ERROR, std::string("Exception during volume change: ") + e.what());
    }
}

float MediaPlayer::getVolume() const {
    return renderer.getVolume();
}

void MediaPlayer::setErrorCallback(std::function<void(const MediaPlayerException&)> callback) {
    ErrorHandler::getInstance().setErrorCallback(std::move(callback));
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
                if (isFullscreen) {
                    toggleFullscreen();
                } else {
                    window.close();
                }
            } else if (event.key.code == sf::Keyboard::F) {
                toggleFullscreen();
            } else if (event.key.code == sf::Keyboard::Up) {
                setVolume(std::min(renderer.getVolume() + 0.1f, 1.0f));  // Increase volume
            } else if (event.key.code == sf::Keyboard::Down) {
                setVolume(std::max(renderer.getVolume() - 0.1f, 0.0f));  // Decrease volume
            } else if (event.key.code == sf::Keyboard::M) {
                // Mute/unmute
                setVolume(renderer.getVolume() > 0.0f ? 0.0f : 1.0f);
            }
        } else if (event.type == sf::Event::Resized) {
            // Update view to match new window size
            sf::FloatRect visibleArea(0, 0, event.size.width, event.size.height);
            window.setView(sf::View(visibleArea));

            // Update controls position
            controls.initialize(sf::Vector2f(event.size.width, event.size.height));
        }

        // Handle control events
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        controls.handleEvent(event, sf::Vector2f(mousePos.x, mousePos.y));
    }
}

void MediaPlayer::update() {
    try {
        // Update controls with current playback position
        double currentPosition = renderer.getCurrentPosition();
        double duration = videoDecoder.getDuration();
        controls.updateProgress(currentPosition, duration);
    } catch (const std::exception& e) {
        ErrorHandler::getInstance().handleError(MediaPlayerException::UNKNOWN_ERROR, std::string("Exception during update: ") + e.what());
    }
}

void MediaPlayer::render() {
    try {
        window.clear(sf::Color::Black);

        // Render video frame
        renderer.render(window);

        // Render controls
        controls.render(window);

        window.display();
    } catch (const std::exception& e) {
        ErrorHandler::getInstance().handleError(MediaPlayerException::RENDER_ERROR, std::string("Exception during rendering: ") + e.what());
    }
}

void MediaPlayer::toggleFullscreen() {
    isFullscreen = !isFullscreen;

    if (isFullscreen) {
        // Save current window size
        originalSize = window.getSize();

        // Switch to fullscreen mode
        window.create(sf::VideoMode::getDesktopMode(), "SFML Video Player", sf::Style::Fullscreen);
    } else {
        // Switch back to windowed mode
        window.create(sf::VideoMode(originalSize.x, originalSize.y), "SFML Video Player");
    }

    window.setFramerateLimit(60);

    // Update controls for new window size
    controls.initialize(sf::Vector2f(window.getSize().x, window.getSize().y));
}

void MediaPlayer::onError(const MediaPlayerException& error) {
    // Default error handler
    std::cerr << "Media Player Error: " << error.what() << std::endl;

    // For critical errors, close the player
    if (error.getCode() == MediaPlayerException::FILE_NOT_FOUND || error.getCode() == MediaPlayerException::DECODER_ERROR) {
        close();
    }
}
