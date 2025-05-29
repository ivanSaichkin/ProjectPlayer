#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <string>
#include <cmath>
#include <filesystem>
#include <vector>
#include <algorithm>

#include "../API/MediaPlayer.hpp"

// UI Constants
const sf::Color BACKGROUND_COLOR(30, 30, 30);
const sf::Color BUTTON_COLOR(60, 60, 60);
const sf::Color BUTTON_HOVER_COLOR(80, 80, 80);
const sf::Color BUTTON_ACTIVE_COLOR(100, 100, 100);
const sf::Color PROGRESS_BAR_BG_COLOR(40, 40, 40);
const sf::Color PROGRESS_BAR_FILL_COLOR(0, 120, 215);
const sf::Color VOLUME_BAR_BG_COLOR(50, 50, 50);
const sf::Color VOLUME_BAR_FILL_COLOR(0, 150, 136);
const sf::Color TEXT_COLOR(220, 220, 220);
const sf::Color SELECTED_FILE_COLOR(70, 120, 200);

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

        isPressed = false;
        animationClock.restart();
        animationDuration = 0.15f; // 150ms animation
    }

    void setPosition(float x, float y) {
        originalPosition = sf::Vector2f(x, y);
        shape.setPosition(x, y);
        centerText();
    }

    void draw(sf::RenderWindow& window) {
        updateAnimation();
        window.draw(shape);
        window.draw(text);
    }

    bool contains(const sf::Vector2f& point) const {
        return shape.getGlobalBounds().contains(point);
    }

    void setHoverState(bool isHovering) {
        if (!isPressed) {
            shape.setFillColor(isHovering ? BUTTON_HOVER_COLOR : BUTTON_COLOR);
        }
    }

    void setActiveState(bool isActive) {
        isPressed = isActive;
        if (isActive) {
            animationClock.restart();
            shape.setFillColor(BUTTON_ACTIVE_COLOR);
        } else {
            shape.setFillColor(BUTTON_COLOR);
        }
    }

private:
    void centerText() {
        sf::FloatRect textBounds = text.getLocalBounds();
        text.setPosition(
            shape.getPosition().x + (shape.getSize().x - textBounds.width) / 2.0f - textBounds.left,
            shape.getPosition().y + (shape.getSize().y - textBounds.height) / 2.0f - textBounds.top - 2.0f
        );
    }

    void updateAnimation() {
        if (isPressed && animationClock.getElapsedTime().asSeconds() < animationDuration) {
            float progress = animationClock.getElapsedTime().asSeconds() / animationDuration;
            float scale = 1.0f - 0.05f * sin(progress * 3.14159f); // Scale down and back up

            sf::Vector2f size = sf::Vector2f(80 * scale, 30 * scale);
            shape.setSize(size);

            // Center the scaled button
            sf::Vector2f offset = sf::Vector2f((80 - size.x) / 2, (30 - size.y) / 2);
            shape.setPosition(originalPosition + offset);

            centerText();
        } else if (!isPressed) {
            shape.setSize(sf::Vector2f(80, 30));
            shape.setPosition(originalPosition);
            centerText();
        }
    }

    sf::RectangleShape shape;
    sf::Text text;
    sf::Vector2f originalPosition;
    bool isPressed;
    sf::Clock animationClock;
    float animationDuration;
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

    void update(double currentTime, double duration, bool isPlaying) {
        this->currentTime = currentTime;
        this->duration = duration;

        // Update fill width based on current position
        float fillWidth = (duration > 0) ? (currentTime / duration) * background.getSize().x : 0;
        fill.setSize(sf::Vector2f(fillWidth, background.getSize().y));

        // Update time text
        timeText.setString(formatTime(currentTime) + " / " + formatTime(duration));
    }

    bool isVideoEnded() const {
        return duration > 0 && currentTime >= duration;
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
        float relativeX = std::max(0.0f, std::min(x - background.getPosition().x, background.getSize().x));
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

class VolumeBar {
public:
    VolumeBar(const sf::Font& font) {
        background.setFillColor(VOLUME_BAR_BG_COLOR);
        fill.setFillColor(VOLUME_BAR_FILL_COLOR);
        handle.setFillColor(sf::Color::White);

        volumeText.setFont(font);
        volumeText.setCharacterSize(12);
        volumeText.setFillColor(TEXT_COLOR);

        volume = 100.0f; // Default volume 100%
        updateDisplay();
    }

    void setSize(float width, float height) {
        background.setSize(sf::Vector2f(width, height));
        handle.setSize(sf::Vector2f(8, height + 4));
    }

    void setPosition(float x, float y) {
        this->x = x;
        this->y = y;
        background.setPosition(x, y);
        fill.setPosition(x, y);
        volumeText.setPosition(x, y - 20);
        updateHandlePosition();
    }

    void update(float newVolume) {
        volume = std::max(0.0f, std::min(100.0f, newVolume));
        updateDisplay();
    }

    void draw(sf::RenderWindow& window) {
        window.draw(background);
        window.draw(fill);
        window.draw(handle);
        window.draw(volumeText);
    }

    bool contains(const sf::Vector2f& point) const {
        sf::FloatRect bounds = background.getGlobalBounds();
        bounds.height += 8; // Увеличить область для удобства
        bounds.top -= 4;
        return bounds.contains(point);
    }

    float getVolumeFromClick(float mouseX) const {
        float relativeX = mouseX - x;
        float ratio = relativeX / background.getSize().x;
        return std::max(0.0f, std::min(1.0f, ratio)) * 100.0f;
    }

    float getVolume() const { return volume; }

private:
    void updateDisplay() {
        float fillWidth = (volume / 100.0f) * background.getSize().x;
        fill.setSize(sf::Vector2f(fillWidth, background.getSize().y));

        volumeText.setString("Volume: " + std::to_string(static_cast<int>(volume)) + "%");
        updateHandlePosition();
    }

    void updateHandlePosition() {
        float handleX = x + (volume / 100.0f) * background.getSize().x - 4;
        handle.setPosition(handleX, y - 2);
    }

    sf::RectangleShape background;
    sf::RectangleShape fill;
    sf::RectangleShape handle;
    sf::Text volumeText;
    float volume;
    float x, y;
};

class FileBrowser {
public:
    FileBrowser(const sf::Font& font) : font(font) {
        background.setFillColor(sf::Color(20, 20, 20, 240));

        title.setFont(font);
        title.setString("Select File from Test Directory");
        title.setCharacterSize(18);
        title.setFillColor(TEXT_COLOR);

        closeButton = std::make_unique<Button>("X", font);
        openButton = std::make_unique<Button>("Open", font);
        cancelButton = std::make_unique<Button>("Cancel", font);

        // Поддерживаемые форматы
        supportedExtensions = {".mp4", ".avi", ".mkv", ".mov", ".wmv", ".flv", ".webm", ".mp3", ".wav", ".ogg", ".m4a"};

        isActive = false;
        selectedIndex = -1;
        scrollOffset = 0;
    }

    void setSize(float width, float height) {
        this->width = width;
        this->height = height;
        background.setSize(sf::Vector2f(width, height));

        closeButton->setPosition(width - 40, 20);
        openButton->setPosition(width - 180, height - 50);
        cancelButton->setPosition(width - 90, height - 50);

        fileListHeight = height - 120; // Высота области списка файлов
    }

    void setPosition(float x, float y) {
        this->x = x;
        this->y = y;
        background.setPosition(x, y);
        title.setPosition(x + 20, y + 20);

        closeButton->setPosition(x + width - 40, y + 20);
        openButton->setPosition(x + width - 180, y + height - 50);
        cancelButton->setPosition(x + width - 90, y + height - 50);
    }

    void show() {
        isActive = true;
        selectedIndex = -1;
        scrollOffset = 0;
        loadCurrentDirectory();
    }

    void draw(sf::RenderWindow& window) {
        if (!isActive) return;

        window.draw(background);
        window.draw(title);

        // Рисуем список файлов
        drawFileList(window);

        closeButton->draw(window);
        openButton->draw(window);
        cancelButton->draw(window);
    }

    void handleEvent(const sf::Event& event, sf::RenderWindow& window) {
        if (!isActive) return;

        if (event.type == sf::Event::MouseMoved) {
            sf::Vector2f mousePos(event.mouseMove.x, event.mouseMove.y);
            closeButton->setHoverState(closeButton->contains(mousePos));
            openButton->setHoverState(openButton->contains(mousePos));
            cancelButton->setHoverState(cancelButton->contains(mousePos));
        }

        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);

            if (closeButton->contains(mousePos) || cancelButton->contains(mousePos)) {
                isActive = false;
                return;
            }

            if (openButton->contains(mousePos) && selectedIndex >= 0) {
                isActive = false;
                if (onFileSelected) {
                    onFileSelected(currentFiles[selectedIndex]);
                }
                return;
            }

            // Проверяем клик по списку файлов
            handleFileListClick(mousePos);
        }

        // Обработка прокрутки
        if (event.type == sf::Event::MouseWheelScrolled) {
            if (event.mouseWheelScroll.x >= x && event.mouseWheelScroll.x <= x + width &&
                event.mouseWheelScroll.y >= y + 60 && event.mouseWheelScroll.y <= y + 60 + fileListHeight) {
                scrollOffset -= event.mouseWheelScroll.delta * 3;
                scrollOffset = std::max(0, std::min(scrollOffset, static_cast<int>(currentFiles.size()) - getVisibleFileCount()));
            }
        }
    }

    bool isVisible() const { return isActive; }

    void setFileSelectedCallback(std::function<void(const std::string&)> callback) {
        onFileSelected = callback;
    }

private:
    void loadCurrentDirectory() {
        currentFiles.clear();
        try {
            // Загружаем файлы из указанной директории
            std::string targetPath = "/Users/andreypavlinich/ProjectPlayer-1/VideoPlayerBack/Test";

            if (!std::filesystem::exists(targetPath)) {
                std::cerr << "Directory does not exist: " << targetPath << std::endl;
                return;
            }

            for (const auto& entry : std::filesystem::directory_iterator(targetPath)) {
                if (entry.is_regular_file()) {
                    std::string filename = entry.path().filename().string();
                    // Показываем все файлы, не только медиа
                    currentFiles.push_back(entry.path().string());
                }
            }

            std::sort(currentFiles.begin(), currentFiles.end());
        } catch (const std::exception& e) {
            std::cerr << "Error loading directory: " << e.what() << std::endl;
        }
    }

    void drawFileList(sf::RenderWindow& window) {
        const float itemHeight = 25.0f;
        const float listY = y + 60;
        int visibleCount = getVisibleFileCount();

        for (int i = 0; i < visibleCount && (i + scrollOffset) < currentFiles.size(); ++i) {
            int fileIndex = i + scrollOffset;
            float itemY = listY + i * itemHeight;

            // Создаем фон для элемента
            sf::RectangleShape itemBg;
            itemBg.setSize(sf::Vector2f(width - 40, itemHeight));
            itemBg.setPosition(x + 20, itemY);

            if (fileIndex == selectedIndex) {
                itemBg.setFillColor(SELECTED_FILE_COLOR);
            } else {
                itemBg.setFillColor(sf::Color(40, 40, 40));
            }

            window.draw(itemBg);

            // Создаем текст файла
            sf::Text fileText;
            fileText.setFont(font);
            fileText.setCharacterSize(12);
            fileText.setFillColor(TEXT_COLOR);

            std::string filename = std::filesystem::path(currentFiles[fileIndex]).filename().string();
            fileText.setString(filename);
            fileText.setPosition(x + 25, itemY + 5);

            window.draw(fileText);
        }
    }

    void handleFileListClick(const sf::Vector2f& mousePos) {
        const float itemHeight = 25.0f;
        const float listY = y + 60;

        if (mousePos.x >= x + 20 && mousePos.x <= x + width - 20 &&
            mousePos.y >= listY && mousePos.y <= listY + fileListHeight) {

            int clickedIndex = static_cast<int>((mousePos.y - listY) / itemHeight) + scrollOffset;
            if (clickedIndex >= 0 && clickedIndex < currentFiles.size()) {
                selectedIndex = clickedIndex;
            }
        }
    }

    int getVisibleFileCount() const {
        return static_cast<int>(fileListHeight / 25.0f);
    }

    sf::RectangleShape background;
    sf::Text title;
    std::unique_ptr<Button> closeButton;
    std::unique_ptr<Button> openButton;
    std::unique_ptr<Button> cancelButton;

    const sf::Font& font;
    std::vector<std::string> currentFiles;
    std::vector<std::string> supportedExtensions;

    float x, y, width, height, fileListHeight;
    bool isActive;
    int selectedIndex;
    int scrollOffset;

    std::function<void(const std::string&)> onFileSelected;
};

int main(int argc, char* argv[]) {
    // Create window
    sf::RenderWindow window(sf::VideoMode(900, 700), "Advanced Media Player");
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
    VolumeBar volumeBar(font);
    FileBrowser fileBrowser(font);

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
    progressBar.setPosition(10, window.getSize().y - 60);

    // Настройка ползунка громкости
    volumeBar.setSize(150, 8);
    volumeBar.setPosition(window.getSize().x - 170, window.getSize().y - 100);

    fileBrowser.setSize(500, 400);
    fileBrowser.setPosition((window.getSize().x - 500) / 2, (window.getSize().y - 400) / 2);

    // Video display area
    sf::RectangleShape videoBackground;
    videoBackground.setFillColor(sf::Color::Black);
    videoBackground.setSize(sf::Vector2f(window.getSize().x - 20, window.getSize().y - 140));
    videoBackground.setPosition(10, 50);

    sf::Texture videoTexture;
    sf::Sprite videoSprite;
    videoSprite.setPosition(10, 50);

    // Set up callbacks
    fileBrowser.setFileSelectedCallback([&](const std::string& filename) {
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
    bool isDraggingVolumeBar = false;
    bool wasVideoEnded = false;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }

            // Handle file browser events
            if (fileBrowser.isVisible()) {
                fileBrowser.handleEvent(event, window);
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
                    if (seekPos >= 0 && seekPos <= player.getDuration()) {
                        // Важно: делаем принудительный flush аудио при перемотке
                        bool wasPlaying = player.isPlaying();
                        player.pause(); // Временно останавливаем
                        player.seek(seekPos);

                        // Даем небольшую задержку для синхронизации
                        sf::sleep(sf::milliseconds(10));

                        // Восстанавливаем состояние воспроизведения
                        if (wasPlaying) {
                            player.play();
                        }

                        std::cout << "Dragging to: " << seekPos << " seconds" << std::endl;
                    }
                }

                // Update volume bar if dragging
                if (isDraggingVolumeBar) {
                    float newVolume = volumeBar.getVolumeFromClick(mousePos.x);
                    volumeBar.update(newVolume);
                    player.setVolume(newVolume / 100.0f);
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
                    double newPos = std::max(0.0, player.getCurrentPosition() - 10.0);

                    // Улучшенная синхронизация для кнопки "назад"
                    bool wasPlaying = player.isPlaying();
                    player.pause();
                    player.seek(newPos);
                    sf::sleep(sf::milliseconds(50)); // Даем время на синхронизацию
                    if (wasPlaying) {
                        player.play();
                    }

                    prevButton.setActiveState(true);
                    std::cout << "Seeking backward to: " << newPos << " seconds" << std::endl;
                } else if (nextButton.contains(mousePos)) {
                    double newPos = std::min(player.getDuration(), player.getCurrentPosition() + 10.0);

                    // Улучшенная синхронизация для кнопки "вперед"
                    bool wasPlaying = player.isPlaying();
                    player.pause();
                    player.seek(newPos);
                    sf::sleep(sf::milliseconds(50)); // Даем время на синхронизацию
                    if (wasPlaying) {
                        player.play();
                    }

                    nextButton.setActiveState(true);
                    std::cout << "Seeking forward to: " << newPos << " seconds" << std::endl;
                } else if (openButton.contains(mousePos)) {
                    fileBrowser.show();
                    openButton.setActiveState(true);
                } else if (progressBar.contains(mousePos)) {
                    isDraggingProgressBar = true;
                    double seekPos = progressBar.getPositionFromClick(mousePos.x);
                    if (seekPos >= 0 && seekPos <= player.getDuration()) {
                        // Улучшенная синхронизация при клике на прогресс-бар
                        bool wasPlaying = player.isPlaying();
                        player.pause();
                        player.seek(seekPos);
                        sf::sleep(sf::milliseconds(30)); // Задержка для синхронизации

                        if (wasPlaying) {
                            player.play();
                        }

                        std::cout << "Seeking to: " << seekPos << " seconds" << std::endl;
                    }
                } else if (volumeBar.contains(mousePos)) {
                    isDraggingVolumeBar = true;
                    float newVolume = volumeBar.getVolumeFromClick(mousePos.x);
                    volumeBar.update(newVolume);
                    player.setVolume(newVolume / 100.0f);
                }
            }

            if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
                // Reset button active states
                playButton.setActiveState(false);
                pauseButton.setActiveState(false);
                prevButton.setActiveState(false);
                nextButton.setActiveState(false);
                openButton.setActiveState(false);

                // При отпускании мыши с прогресс-бара делаем финальную синхронизацию
                if (isDraggingProgressBar) {
                    // Финальная синхронизация при завершении перетаскивания
                    player.update(); // Принудительное обновление
                    sf::sleep(sf::milliseconds(20));
                }

                isDraggingProgressBar = false;
                isDraggingVolumeBar = false;
            }

            // Keyboard shortcuts
            if (event.type == sf::Event::KeyPressed) {
                switch (event.key.code) {
                    case sf::Keyboard::Space:
                        player.togglePlayPause();
                        break;
                    case sf::Keyboard::Left:
                        {
                            double newPos = std::max(0.0, player.getCurrentPosition() - 5.0);

                            // Улучшенная синхронизация для клавиш
                            bool wasPlaying = player.isPlaying();
                            player.pause();
                            player.seek(newPos);
                            sf::sleep(sf::milliseconds(50));
                            if (wasPlaying) {
                                player.play();
                            }

                            std::cout << "Seeking backward (keyboard) to: " << newPos << " seconds" << std::endl;
                        }
                        break;
                    case sf::Keyboard::Right:
                        {
                            double newPos = std::min(player.getDuration(), player.getCurrentPosition() + 5.0);

                            // Улучшенная синхронизация для клавиш
                            bool wasPlaying = player.isPlaying();
                            player.pause();
                            player.seek(newPos);
                            sf::sleep(sf::milliseconds(50));
                            if (wasPlaying) {
                                player.play();
                            }

                            std::cout << "Seeking forward (keyboard) to: " << newPos << " seconds" << std::endl;
                        }
                        break;
                    case sf::Keyboard::O:
                        if (event.key.control) {
                            fileBrowser.show();
                        }
                        break;
                    case sf::Keyboard::Up:
                        {
                            float newVolume = std::min(100.0f, volumeBar.getVolume() + 5.0f);
                            volumeBar.update(newVolume);
                            player.setVolume(newVolume / 100.0f);
                        }
                        break;
                    case sf::Keyboard::Down:
                        {
                            float newVolume = std::max(0.0f, volumeBar.getVolume() - 5.0f);
                            volumeBar.update(newVolume);
                            player.setVolume(newVolume / 100.0f);
                        }
                        break;
                    default:
                        break;
                }
            }
        }

        // Update player - важно делать это регулярно для поддержания синхронизации
        player.update();

        // Check if video ended and restart if needed
        bool isVideoEnded = progressBar.isVideoEnded();
        if (isVideoEnded && !wasVideoEnded && player.getDuration() > 0) {
            // Video just ended, restart from beginning
            player.seek(0.0);
            sf::sleep(sf::milliseconds(100)); // Даем время на синхронизацию
            player.play();
            std::cout << "Video ended, restarting from beginning" << std::endl;
        }
        wasVideoEnded = isVideoEnded;

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

        // Update progress bar with playing state
        progressBar.update(player.getCurrentPosition(), player.getDuration(), player.isPlaying());

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
        volumeBar.draw(window);

        // Draw file browser (if visible)
        fileBrowser.draw(window);

        // Display window
        window.display();
    }

    // Clean up
    player.close();

    return 0;
}
