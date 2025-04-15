#include "../include/PlayerControls.hpp"
#include <iostream>

PlayerControls::PlayerControls()
    : isDragging(false),
      progress(0.0),
      duration(0.0),
      isPlaying(false) {
}

void PlayerControls::initialize(const sf::Vector2f& size) {
    // Load button textures
    if (!playTexture.loadFromFile("play.png") || !pauseTexture.loadFromFile("pause.png")) {
        // Fallback: create simple textures
        playTexture.create(32, 32);
        pauseTexture.create(32, 32);

        sf::Image playImage;
        playImage.create(32, 32, sf::Color::Transparent);
        for (int y = 8; y < 24; ++y) {
            for (int x = 8; x < 24; ++x) {
                if (x < 16 + (y - 16) || x < 16 - (y - 16)) {
                    playImage.setPixel(x, y, sf::Color::White);
                }
            }
        }
        playTexture.update(playImage);

        sf::Image pauseImage;
        pauseImage.create(32, 32, sf::Color::Transparent);
        for (int y = 8; y < 24; ++y) {
            for (int x = 8; x < 24; ++x) {
                if ((x >= 10 && x <= 14) || (x >= 18 && x <= 22)) {
                    pauseImage.setPixel(x, y, sf::Color::White);
                }
            }
        }
        pauseTexture.update(pauseImage);
    }

    // Set up background
    background.setSize(sf::Vector2f(size.x, 40));
    background.setPosition(0, size.y - 40);
    background.setFillColor(sf::Color(30, 30, 30, 200));

    // Set up progress bar
    progressBar.setSize(sf::Vector2f(size.x - 80, 10));
    progressBar.setPosition(70, size.y - 25);
    progressBar.setFillColor(sf::Color(60, 60, 60));

    progressBarFill.setSize(sf::Vector2f(0, 10));
    progressBarFill.setPosition(70, size.y - 25);
    progressBarFill.setFillColor(sf::Color(100, 100, 255));

    // Set up play/pause button
    playPauseButton.setSize(sf::Vector2f(32, 32));
    playPauseButton.setPosition(20, size.y - 36);
    playPauseButton.setFillColor(sf::Color::Transparent);

    buttonSprite.setTexture(playTexture);
    buttonSprite.setPosition(20, size.y - 36);

    isPlaying = false;
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

        // Check if progress bar was clicked
        if (contains(progressBar, mousePosition)) {
            isDragging = true;

            // Calculate new position
            float relativeX = mousePosition.x - progressBar.getPosition().x;
            float ratio = relativeX / progressBar.getSize().x;
            progress = ratio * duration;

            // Update progress bar fill
            progressBarFill.setSize(sf::Vector2f(relativeX, 10));

            // Seek to new position
            if (onSeek) {
                onSeek(progress);
            }
        }
    } else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        isDragging = false;
    } else if (event.type == sf::Event::MouseMoved && isDragging) {
        // Update progress while dragging
        float relativeX = mousePosition.x - progressBar.getPosition().x;
        relativeX = std::max(0.0f, std::min(relativeX, progressBar.getSize().x));

        float ratio = relativeX / progressBar.getSize().x;
        progress = ratio * duration;

        // Update progress bar fill
        progressBarFill.setSize(sf::Vector2f(relativeX, 10));

        // Seek to new position
        if (onSeek) {
            onSeek(progress);
        }
    }
}

void PlayerControls::render(sf::RenderTarget& target) {
    target.draw(background);
    target.draw(progressBar);
    target.draw(progressBarFill);
    target.draw(playPauseButton);
    target.draw(buttonSprite);
}

void PlayerControls::setPlayPauseCallback(std::function<void()> callback) {
    onPlayPause = callback;
}

void PlayerControls::setSeekCallback(std::function<void(double)> callback) {
    onSeek = callback;
}

void PlayerControls::updateProgress(double current, double total) {
    if (!isDragging) {
        progress = current;
        duration = total;

        float ratio = (duration > 0) ? (progress / duration) : 0.0f;
        float width = ratio * progressBar.getSize().x;

        progressBarFill.setSize(sf::Vector2f(width, 10));
    }
}

bool PlayerControls::contains(const sf::RectangleShape& rect, const sf::Vector2f& point) const {
    sf::FloatRect bounds = rect.getGlobalBounds();
    return bounds.contains(point);
}
