
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <string>
#include <cmath>
#include <filesystem>

#include "../API/MediaPlayer.hpp"

// UI Constants
const sf::Color BACKGROUND_COLOR(30, 30, 30);
const sf::Color BUTTON_COLOR(60, 60, 60);
const sf::Color BUTTON_HOVER_COLOR(80, 80, 80);
const sf::Color BUTTON_ACTIVE_COLOR(100, 100, 100);
const sf::Color PROGRESS_BAR_BG_COLOR(40, 40, 40);
const sf::Color PROGRESS_BAR_FILL_COLOR(0, 120, 215);
const sf::Color TEXT_COLOR(220, 220, 220);

// UI Components
class Button {
public:
    Button(const std::string& label, const sf::Font& font) {
        shape.setSize(sf::Vector2f(80, 30));
        shape.setFillColor(BUTTON_COLOR);

        text.setFont(font);
        text.setString(label);
        text.setCharacterSize(14);
        text.setFillColor(TEXT_COLOR);
    }

    void setPosition(float x, float y) {
        shape.setPosition(x, y);
        centerText();
    }

    void draw(sf::RenderWindow& window) {
        window.draw(shape);
        window.draw(text);
    }

    bool contains(const sf::Vector2f& point) const {
        return shape.getGlobalBounds().contains(point);
    }

    void setHoverState(bool isHovering) {
        shape.setFillColor(isHovering ? BUTTON_HOVER_COLOR : BUTTON_COLOR);
    }

    void setActiveState(bool isActive) {
        shape.setFillColor(isActive ? BUTTON_ACTIVE_COLOR : BUTTON_COLOR);
    }

private:
    void centerText() {
        sf::FloatRect textBounds = text.getLocalBounds();
        text.setPosition(
            shape.getPosition().x + (shape.getSize().x - textBounds.width) / 2.0f - textBounds.left,
            shape.getPosition().y + (shape.getSize().y - textBounds.height) / 2.0f - textBounds.top - 2.0f
        );
    }

    sf::RectangleShape shape;
    sf::Text text;
};

class ProgressBar {
public:
    ProgressBar(const sf::Font& font) {
        background.setFillColor(PROGRESS_BAR_BG_COLOR);
        fill.setFillColor(PROGRESS_BAR_FILL_COLOR);

        timeText.setFont(font);
        timeText.setCharacterSize(12);
        timeText.setFillColor(TEXT_COLOR);
        timeText.setString("00:00 / 00:00");
    }

    void setSize(float width, float height) {
        background.setSize(sf::Vector2f(width, height));
    }

    void setPosition(float x, float y) {
        background.setPosition(x, y);
        fill.setPosition(x, y);
        timeText.setPosition(x, y + background.getSize().y + 5.0f);
    }

    void update(double currentTime, double duration) {
        this->currentTime = currentTime;
        this->duration = duration;

        // Update fill width based on current position
        float fillWidth = (duration > 0) ? (currentTime / duration) * background.getSize().x : 0;
        fill.setSize(sf::Vector2f(fillWidth, background.getSize().y));

        // Update time text
        timeText.setString(formatTime(currentTime) + " / " + formatTime(duration));
    }

    void draw(sf::RenderWindow& window) {
        window.draw(background);
        window.draw(fill);
        window.draw(timeText);
    }

    bool contains(const sf::Vector2f& point) const {
        return background.getGlobalBounds().contains(point);
    }

    double getPositionFromClick(float x) const {
        float relativeX = x - background.getPosition().x;
        float ratio = relativeX / background.getSize().x;
        return ratio * duration;
    }

private:
    std::string formatTime(double seconds) {
        int mins = static_cast<int>(seconds) / 60;
        int secs = static_cast<int>(seconds) % 60;
        return std::to_string(mins) + ":" + (secs < 10 ? "0" : "") + std::to_string(secs);
    }

    sf::RectangleShape background;
    sf::RectangleShape fill;
    sf::Text timeText;
    double currentTime = 0.0;
    double duration = 0.0;
};

class FileDialog {
public:
    FileDialog(const sf::Font& font) {
        background.setFillColor(sf::Color(20, 20, 20, 230));

        title.setFont(font);
        title.setString("Open Media File");
        title.setCharacterSize(18);
        title.setFillColor(TEXT_COLOR);

        closeButton = std::make_unique<Button>("X", font);
        okButton = std::make_unique<Button>("Open", font);
        cancelButton = std::make_unique<Button>("Cancel", font);

        inputBox.setFillColor(sf::Color(50, 50, 50));
        inputBox.setOutlineColor(sf::Color(100, 100, 100));
        inputBox.setOutlineThickness(1);

        inputText.setFont(font);
        inputText.setCharacterSize(14);
        inputText.setFillColor(TEXT_COLOR);
        inputText.setString("");

        cursorShape.setFillColor(TEXT_COLOR);

        isActive = false;
        cursorVisible = true;
        cursorClock.restart();
    }

    void setSize(float width, float height) {
        background.setSize(sf::Vector2f(width, height));
        inputBox.setSize(sf::Vector2f(width - 40, 30));

        closeButton->setPosition(width - 40, 20);
        okButton->setPosition(width - 180, height - 50);
        cancelButton->setPosition(width - 90, height - 50);
    }

    void setPosition(float x, float y) {
        background.setPosition(x, y);
        title.setPosition(x + 20, y + 20);
        inputBox.setPosition(x + 20, y + 60);
        inputText.setPosition(x + 25, y + 65);
        cursorShape.setPosition(inputText.getPosition().x + inputText.getLocalBounds().width + 2, y + 65);
        cursorShape.setSize(sf::Vector2f(1, 20));

        closeButton->setPosition(x + background.getSize().x - 40, y + 20);
        okButton->setPosition(x + background.getSize().x - 180, y + background.getSize().y - 50);
        cancelButton->setPosition(x + background.getSize().x - 90, y + background.getSize().y - 50);
    }

    void draw(sf::RenderWindow& window) {
        if (!isActive) return;

        window.draw(background);
        window.draw(title);
        window.draw(inputBox);
        window.draw(inputText);

        // Blink cursor
        if (cursorClock.getElapsedTime().asSeconds() > 0.5f) {
            cursorVisible = !cursorVisible;
            cursorClock.restart();
        }

        if (cursorVisible) {
            window.draw(cursorShape);
        }

        closeButton->draw(window);
        okButton->draw(window);
        cancelButton->draw(window);
    }

    void handleEvent(const sf::Event& event, sf::RenderWindow& window) {
        if (!isActive) return;

        if (event.type == sf::Event::MouseMoved) {
            sf::Vector2f mousePos(event.mouseMove.x, event.mouseMove.y);
            closeButton->setHoverState(closeButton->contains(mousePos));
            okButton->setHoverState(okButton->contains(mousePos));
            cancelButton->setHoverState(cancelButton->contains(mousePos));
        }

        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);

            if (closeButton->contains(mousePos) || cancelButton->contains(mousePos)) {
                isActive = false;
                return;
            }

            if (okButton->contains(mousePos)) {
                isActive = false;
                if (onFileSelected && !filePath.empty()) {
                    onFileSelected(filePath);
                }
                return;
            }
        }

        if (event.type == sf::Event::TextEntered) {
            if (event.text.unicode == 8) { // Backspace
                if (!filePath.empty()) {
                    filePath.pop_back();
                }
            } else if (event.text.unicode >= 32) { // Printable characters
                filePath += static_cast<char>(event.text.unicode);
            }

            inputText.setString(filePath);
            cursorShape.setPosition(inputText.getPosition().x + inputText.getLocalBounds().width + 2,
                                   cursorShape.getPosition().y);
        }
    }

    void show() {
        isActive = true;
        filePath = "";
        inputText.setString(filePath);
        cursorVisible = true;
        cursorClock.restart();
    }

    bool isVisible() const {
        return isActive;
    }

    void setFileSelectedCallback(std::function<void(const std::string&)> callback) {
        onFileSelected = callback;
    }

private:
    sf::RectangleShape background;
    sf::Text title;
    std::unique_ptr<Button> closeButton;
    std::unique_ptr<Button> okButton;
    std::unique_ptr<Button> cancelButton;
    sf::RectangleShape inputBox;
    sf::Text inputText;
    sf::RectangleShape cursorShape;
    sf::Clock cursorClock;

    std::string filePath;
    bool isActive;
    bool cursorVisible;

    std::function<void(const std::string&)> onFileSelected;
};

int main(int argc, char* argv[]) {
    // Create window
    sf::RenderWindow window(sf::VideoMode(800, 600), "Media Player");
    window.setFramerateLimit(60);

    // Load font
    sf::Font font;
    if (!font.loadFromFile("/Users/andreypavlinich/ProjectPlayer-1/VideoPlayerBack/Test/Arial.ttf")) {
        std::cerr << "Error: Could not load font" << std::endl;
        return 1;
    }

    // Create media player
    MediaPlayer player;

    // Create UI components
    Button playButton("Play", font);
    Button pauseButton("Pause", font);
    Button prevButton("Prev", font);
    Button nextButton("Next", font);
    Button openButton("Open", font);

    ProgressBar progressBar(font);
    FileDialog fileDialog(font);

    // Set up UI layout
    const float buttonY = 10;
    const float buttonSpacing = 10;
    float currentX = 10;

    playButton.setPosition(currentX, buttonY);
    currentX += 90;

    pauseButton.setPosition(currentX, buttonY);
    currentX += 90;

    prevButton.setPosition(currentX, buttonY);
    currentX += 90;

    nextButton.setPosition(currentX, buttonY);
    currentX += 90;

    openButton.setPosition(currentX, buttonY);

    progressBar.setSize(window.getSize().x - 20, 10);
    progressBar.setPosition(10, window.getSize().y - 40);

    fileDialog.setSize(400, 200);
    fileDialog.setPosition((window.getSize().x - 400) / 2, (window.getSize().y - 200) / 2);

    // Video display area
    sf::RectangleShape videoBackground;
    videoBackground.setFillColor(sf::Color::Black);
    videoBackground.setSize(sf::Vector2f(window.getSize().x - 20, window.getSize().y - 100));
    videoBackground.setPosition(10, 50);

    sf::Texture videoTexture;
    sf::Sprite videoSprite;
    videoSprite.setPosition(10, 50);

    // Set up callbacks
    fileDialog.setFileSelectedCallback([&](const std::string& filename) {
        if (player.open(filename)) {
            std::cout << "Opened: " << filename << std::endl;
            player.play();
        } else {
            std::cerr << "Failed to open: " << filename << std::endl;
        }
    });

    // Try to open file from command line argument
    if (argc > 1) {
        std::string filename = argv[1];
        if (player.open(filename)) {
            std::cout << "Opened: " << filename << std::endl;
            player.play();
        } else {
            std::cerr << "Failed to open: " << filename << std::endl;
        }
    }

    // Main loop
    sf::Clock clock;
    bool isDraggingProgressBar = false;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }

            // Handle file dialog events
            if (fileDialog.isVisible()) {
                fileDialog.handleEvent(event, window);
                continue;
            }

            if (event.type == sf::Event::MouseMoved) {
                sf::Vector2f mousePos(event.mouseMove.x, event.mouseMove.y);

                // Update button hover states
                playButton.setHoverState(playButton.contains(mousePos));
                pauseButton.setHoverState(pauseButton.contains(mousePos));
                prevButton.setHoverState(prevButton.contains(mousePos));
                nextButton.setHoverState(nextButton.contains(mousePos));
                openButton.setHoverState(openButton.contains(mousePos));

                // Update progress bar if dragging
                if (isDraggingProgressBar) {
                    double seekPos = progressBar.getPositionFromClick(mousePos.x);
                    seekPos = std::max(0.0, std::min(seekPos, player.getDuration()));
                    player.seek(seekPos);
                }
            }

            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);

                if (playButton.contains(mousePos)) {
                    player.play();
                    playButton.setActiveState(true);
                } else if (pauseButton.contains(mousePos)) {
                    player.pause();
                    pauseButton.setActiveState(true);
            } else if (prevButton.contains(mousePos)) {
                player.seek(std::max(0.0, player.getCurrentPosition() - 10.0));
                prevButton.setActiveState(true);
            } else if (nextButton.contains(mousePos)) {
                player.seek(std::min(player.getDuration(), player.getCurrentPosition() + 10.0));
                nextButton.setActiveState(true);
            } else if (openButton.contains(mousePos)) {
                fileDialog.show();
                openButton.setActiveState(true);
            } else if (progressBar.contains(mousePos)) {
                isDraggingProgressBar = true;
                double seekPos = progressBar.getPositionFromClick(mousePos.x);
                seekPos = std::max(0.0, std::min(seekPos, player.getDuration()));
                player.seek(seekPos);
            }
        }

        if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
            // Reset button active states
            playButton.setActiveState(false);
            pauseButton.setActiveState(false);
            prevButton.setActiveState(false);
            nextButton.setActiveState(false);
            openButton.setActiveState(false);
            isDraggingProgressBar = false;
        }

        // Keyboard shortcuts
        if (event.type == sf::Event::KeyPressed) {
            switch (event.key.code) {
                case sf::Keyboard::Space:
                    player.togglePlayPause();
                    break;
                case sf::Keyboard::Left:
                    player.seek(std::max(0.0, player.getCurrentPosition() - 5.0));
                    break;
                case sf::Keyboard::Right:
                    player.seek(std::min(player.getDuration(), player.getCurrentPosition() + 5.0));
                    break;
                case sf::Keyboard::O:
                    if (event.key.control) {
                        fileDialog.show();
                    }
                    break;
                default:
                    break;
            }
        }
    }

    // Update player
    player.update();

    // Get current video frame
    if (player.getCurrentFrame(videoTexture)) {
        videoSprite.setTexture(videoTexture, true);

        // Center video in the video area
        sf::Vector2u textureSize = videoTexture.getSize();
        sf::Vector2f videoAreaSize = videoBackground.getSize();

        float scaleX = videoAreaSize.x / textureSize.x;
        float scaleY = videoAreaSize.y / textureSize.y;
        float scale = std::min(scaleX, scaleY);

        videoSprite.setScale(scale, scale);

        float videoWidth = textureSize.x * scale;
        float videoHeight = textureSize.y * scale;
        float videoX = videoBackground.getPosition().x + (videoAreaSize.x - videoWidth) / 2;
        float videoY = videoBackground.getPosition().y + (videoAreaSize.y - videoHeight) / 2;

        videoSprite.setPosition(videoX, videoY);
    }

    // Update progress bar
    progressBar.update(player.getCurrentPosition(), player.getDuration());

    // Clear window
    window.clear(BACKGROUND_COLOR);

    // Draw video background and video
    window.draw(videoBackground);
    if (videoTexture.getSize().x > 0 && videoTexture.getSize().y > 0) {
        window.draw(videoSprite);
    }

    // Draw UI components
    playButton.draw(window);
    pauseButton.draw(window);
    prevButton.draw(window);
    nextButton.draw(window);
    openButton.draw(window);
    progressBar.draw(window);

    // Draw file dialog (if visible)
    fileDialog.draw(window);

    // Display window
    window.display();
}

// Clean up
player.close();

return 0;
}
