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
    void setVolumeCallback(std::function<void(float)> callback);

    // Update progress bar
    void updateProgress(double current, double total);

    // Update volume display
    void updateVolume(float volume);

private:
    sf::RectangleShape background;
    sf::RectangleShape progressBar;
    sf::RectangleShape progressBarFill;
    sf::RectangleShape playPauseButton;

    // Volume control elements
    sf::RectangleShape volumeBar;
    sf::RectangleShape volumeBarFill;
    sf::RectangleShape volumeIcon;
    sf::RectangleShape muteIcon;

    sf::Texture playTexture;
    sf::Texture pauseTexture;
    sf::Sprite buttonSprite;

    sf::Font font;
    sf::Text volumeText;
    sf::Text timeText;

    bool isDraggingProgress;
    bool isDraggingVolume;
    double progress;
    double duration;
    float volume;
    bool isPlaying;
    bool isMuted;

    std::function<void()> onPlayPause;
    std::function<void(double)> onSeek;
    std::function<void(float)> onVolume;

    // Check if point is inside a rectangle
    bool contains(const sf::RectangleShape& rect, const sf::Vector2f& point) const;

    // Format time as MM:SS
    std::string formatTime(double seconds) const;

    // Create simple textures for buttons
    void createDefaultTextures();
};
