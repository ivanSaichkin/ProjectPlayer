#include "../include/PlayerControls.hpp"
#include "../include/ErrorHandler.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>

PlayerControls::PlayerControls()
    : isDraggingProgress(false),
      isDraggingVolume(false),
      progress(0.0),
      duration(0.0),
      volume(1.0f),
      isPlaying(false),
      isMuted(false) {
}

void PlayerControls::initialize(const sf::Vector2f& size) {
    // Try to load font
    if (!font.loadFromFile("arial.ttf")) {
        // Use system font as fallback
        #ifdef _WIN32
        if (!font.loadFromFile("C:\\Windows\\Fonts\\arial.ttf")) {
            std::cerr << "Warning: Could not load font, text will not be displayed" << std::endl;
        }
        #elif __APPLE__
        if (!font.loadFromFile("/Library/Fonts/Arial.ttf")) {
            std::cerr << "Warning: Could not load font, text will not be displayed" << std::endl;
        }
        #else
        if (!font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")) {
            std::cerr << "Warning: Could not load font, text will not be displayed" << std::endl;
        }
        #endif
    }

    // Load button textures
    if (!playTexture.loadFromFile("play.png") || !pauseTexture.loadFromFile("pause.png")) {
        // Fallback: create simple textures
        createDefaultTextures();
    }

    // Set up background
    background.setSize(sf::Vector2f(size.x, 50));
    background.setPosition(0, size.y - 50);
    background.setFillColor(sf::Color(30, 30, 30, 220));

    // Set up progress bar
    progressBar.setSize(sf::Vector2f(size.x - 200, 10));
    progressBar.setPosition(70, size.y - 30);
    progressBar.setFillColor(sf::Color(60, 60, 60));

    progressBarFill.setSize(sf::Vector2f(0, 10));
    progressBarFill.setPosition(70, size.y - 30);
    progressBarFill.setFillColor(sf::Color(100, 100, 255));

    // Set up play/pause button
    playPauseButton.setSize(sf::Vector2f(32, 32));
    playPauseButton.setPosition(20, size.y - 41);
    playPauseButton.setFillColor(sf::Color::Transparent);

    buttonSprite.setTexture(playTexture);
    buttonSprite.setPosition(20, size.y - 41);

    // Set up volume control
    volumeBar.setSize(sf::Vector2f(80, 6));
    volumeBar.setPosition(size.x - 100, size.y - 28);
    volumeBar.setFillColor(sf::Color(60, 60, 60));

    volumeBarFill.setSize(sf::Vector2f(80, 6));
    volumeBarFill.setPosition(size.x - 100, size.y - 28);
    volumeBarFill.setFillColor(sf::Color(100, 255, 100));

    volumeIcon.setSize(sf::Vector2f(20, 20));
    volumeIcon.setPosition(size.x - 130, size.y - 35);
    volumeIcon.setFillColor(sf::Color(200, 200, 200));

    // Set up time text
    timeText.setFont(font);
    timeText.setCharacterSize(14);
    timeText.setFillColor(sf::Color::White);
    timeText.setPosition(progressBar.getPosition().x, size.y - 45);

    // Set up volume text
    volumeText.setFont(font);
    volumeText.setCharacterSize(14);
    volumeText.setFillColor(sf::Color::White);
    volumeText.setPosition(volumeBar.getPosition().x + volumeBar.getSize().x + 5, size.y - 30);

    isPlaying = false;
    updateVolume(1.0f);
}

void PlayerControls::handleEvent(const sf::Event& event, const sf::Vector2f& mousePosition) {
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        // Check if play/pause button was clicked
        if (contains(playPauseButton, mousePosition)) {
            isPlaying = !isPlaying;
            buttonSprite.setTexture(isPlaying ? pauseTexture : playTexture);

            if (onPlayPause) {
                onPlayPause();
            }
        }

        // Check if volume icon was clicked (mute/unmute)
        if (contains(volumeIcon, mousePosition)) {
            isMuted = !isMuted;
            float newVolume = isMuted ? 0.0f : volume;
            updateVolume(newVolume);

            if (onVolume) {
                onVolume(newVolume);
            }
        }

        // Check if progress bar was clicked
        if (contains(progressBar, mousePosition)) {
            isDraggingProgress = true;

            // Calculate new position
            float relativeX = mousePosition.x - progressBar.getPosition().x;
            float ratio = relativeX / progressBar.getSize().x;
            ratio = std::max(0.0f, std::min(ratio, 1.0f));
            progress = ratio * duration;

            // Update progress bar fill
            progressBarFill.setSize(sf::Vector2f(relativeX, 10));

            // Update time text
            timeText.setString(formatTime(progress) + " / " + formatTime(duration));

            // Seek to new position
            if (onSeek) {
                onSeek(progress);
            }
        }

        // Check if volume bar was clicked
        if (contains(volumeBar, mousePosition)) {
            isDraggingVolume = true;

            // Calculate new volume
            float relativeX = mousePosition.x - volumeBar.getPosition().x;
            float ratio = relativeX / volumeBar.getSize().x;
            ratio = std::max(0.0f, std::min(ratio, 1.0f));

            updateVolume(ratio);
            isMuted = (ratio == 0.0f);

            if (onVolume) {
                onVolume(ratio);
            }
        }
    } else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        isDraggingProgress = false;
        isDraggingVolume = false;
    } else if (event.type == sf::Event::MouseMoved) {
        if (isDraggingProgress) {
            // Update progress while dragging
            float relativeX = mousePosition.x - progressBar.getPosition().x;
            relativeX = std::max(0.0f, std::min(relativeX, progressBar.getSize().x));

            float ratio = relativeX / progressBar.getSize().x;
            progress = ratio * duration;

            // Update progress bar fill
            progressBarFill.setSize(sf::Vector2f(relativeX, 10));

            // Update time text
            timeText.setString(formatTime(progress) + " / " + formatTime(duration));

            // Seek to new position
            if (onSeek) {
                onSeek(progress);
            }
        }

        if (isDraggingVolume) {
            // Update volume while dragging
            float relativeX = mousePosition.x - volumeBar.getPosition().x;
            relativeX = std::max(0.0f, std::min(relativeX, volumeBar.getSize().x));

            float ratio = relativeX / volumeBar.getSize().x;
            updateVolume(ratio);
            isMuted = (ratio == 0.0f);

            if (onVolume) {
                onVolume(ratio);
            }
        }
    }
}

void PlayerControls::render(sf::RenderTarget& target) {
    target.draw(background);
    target.draw(progressBar);
    target.draw(progressBarFill);
    target.draw(playPauseButton);
    target.draw(buttonSprite);
    target.draw(volumeBar);
    target.draw(volumeBarFill);
    target.draw(volumeIcon);
    target.draw(timeText);
    target.draw(volumeText);
}

void PlayerControls::setPlayPauseCallback(std::function<void()> callback) {
    onPlayPause = std::move(callback);
}

void PlayerControls::setSeekCallback(std::function<void(double)> callback) {
    onSeek = std::move(callback);
}

void PlayerControls::setVolumeCallback(std::function<void(float)> callback) {
    onVolume = std::move(callback);
}

void PlayerControls::updateProgress(double current, double total) {
    if (!isDraggingProgress) {
        progress = current;
        duration = total;

        float ratio = (duration > 0) ? (progress / duration) : 0.0f;
        float width = ratio * progressBar.getSize().x;

        progressBarFill.setSize(sf::Vector2f(width, 10));

        // Update time text
        timeText.setString(formatTime(progress) + " / " + formatTime(duration));
    }
}

void PlayerControls::updateVolume(float newVolume) {
    volume = newVolume;

    // Update volume bar fill
    float width = volume * volumeBar.getSize().x;
    volumeBarFill.setSize(sf::Vector2f(width, 6));

    // Update volume text (percentage)
    int volumePercent = static_cast<int>(volume * 100);
    volumeText.setString(std::to_string(volumePercent) + "%");

    // Update volume icon color based on mute state
    if (isMuted || volume < 0.01f) {
        volumeIcon.setFillColor(sf::Color(150, 150, 150));
    } else {
        volumeIcon.setFillColor(sf::Color(200, 200, 200));
    }
}

bool PlayerControls::contains(const sf::RectangleShape& rect, const sf::Vector2f& point) const {
    sf::FloatRect bounds = rect.getGlobalBounds();
    return bounds.contains(point);
}

std::string PlayerControls::formatTime(double seconds) const {
    int totalSeconds = static_cast<int>(seconds);
    int mins = totalSeconds / 60;
    int secs = totalSeconds % 60;

    std::stringstream ss;
    ss << std::setfill('0') << std::setw(2) << mins << ":"
       << std::setfill('0') << std::setw(2) << secs;
    return ss.str();
}

void PlayerControls::createDefaultTextures() {
    // Create play button texture
    playTexture.create(32, 32);
    sf::Image playImage;
    playImage.create(32, 32, sf::Color::Transparent);

    // Draw a triangle for play
    for (int y = 8; y < 24; ++y) {
        for (int x = 12; x < 22; ++x) {
            if (x < 12 + (y - 8) && x < 12 + (32 - y - 8)) {
                playImage.setPixel(x, y, sf::Color::White);
            }
        }
    }
    playTexture.update(playImage);

    // Create pause button texture
    pauseTexture.create(32, 32);
    sf::Image pauseImage;
    pauseImage.create(32, 32, sf::Color::Transparent);

    // Draw two vertical bars for pause
    for (int y = 8; y < 24; ++y) {
        for (int x = 10; x < 22; ++x) {
            if ((x >= 10 && x <= 14) || (x >= 18 && x <= 22)) {
                pauseImage.setPixel(x, y, sf::Color::White);
            }
        }
    }
    pauseTexture.update(pauseImage);
}
