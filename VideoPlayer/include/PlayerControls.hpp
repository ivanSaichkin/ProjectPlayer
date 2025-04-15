#pragma once

#include <SFML/Graphics.hpp>
#include <functional>

class PlayerControls {
public:
    PlayerControls();

    void initialize(const sf::Vector2f& size);
    void handleEvent(const sf::Event& event, const sf::Vector2f& mousePosition);
    void render(sf::RenderTarget& target);

    // Set callbacks for controls
    void setPlayPauseCallback(std::function<void()> callback);
    void setSeekCallback(std::function<void(double)> callback);

    // Update progress bar
    void updateProgress(double current, double total);

private:
    sf::RectangleShape background;
    sf::RectangleShape progressBar;
    sf::RectangleShape progressBarFill;
    sf::RectangleShape playPauseButton;

    sf::Texture playTexture;
    sf::Texture pauseTexture;
    sf::Sprite buttonSprite;

    bool isDragging;
    double progress;
    double duration;
    bool isPlaying;

    std::function<void()> onPlayPause;
    std::function<void(double)> onSeek;

    // Check if point is inside a rectangle
    bool contains(const sf::RectangleShape& rect, const sf::Vector2f& point) const;
};
