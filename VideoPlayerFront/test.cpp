#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <iostream>
#include <functional>
#include <memory>
#include <string>

#include "/Users/andreypavlinich/ProjectPlayer-1/VideoPlayerBack/include/core/MediaPlayerManager.hpp"

class VideoPlayerGUI {
private:
    // SFML components
    sf::RenderWindow window;
    sf::RenderTexture videoTexture;
    sf::Sprite videoSprite;
    sf::Font font;
    sf::Clock clock;

    // GUI components
    struct Button {
        sf::RectangleShape shape;
        sf::Text text;
        std::function<void()> onClick;
        bool isHovered = false;
    };

    struct Slider {
        sf::RectangleShape track;
        sf::RectangleShape thumb;
        bool isDragging = false;
        float value = 0.0f; // 0.0 to 1.0
        std::function<void(float)> onChange;
    };

    // Layout components
    sf::RectangleShape videoArea;
    sf::RectangleShape controlPanel;
    sf::RectangleShape playlistPanel;

    // Control buttons
    Button playPauseButton;
    Button stopButton;
    Button prevButton;
    Button nextButton;
    Button muteButton;
    Button addFileButton;

    // Sliders
    Slider timelineSlider;
    Slider volumeSlider;

    // Playlist display
    std::vector<Button> playlistItems;
    sf::RectangleShape playlistBackground;
    sf::View playlistView;
    float playlistScrollPosition = 0;

    // File dialog (simplified)
    bool showFileDialog = false;
    sf::RectangleShape fileDialogBackground;
    sf::Text fileDialogTitle;
    std::vector<Button> fileDialogItems;
    Button fileDialogCloseButton;
    std::string currentDirectory;

    // MediaPlayerManager reference
    VideoPlayer::Core::MediaPlayerManager& mediaManager;

    // Current state tracking
    bool isPlaying = false;
    bool userIsSeeking = false;
    double currentMediaDuration = 0.0;

    // Initialize GUI components
    void initWindow() {
        window.create(sf::VideoMode(1280, 720), "SFML Video Player");
        window.setFramerateLimit(60);
        videoTexture.create(1280, 720);
    }

    void initFont() {
        if (!font.loadFromFile("/System/Library/Fonts/Supplemental/Arial.ttf")) {
            // Fallback to default font if available
            std::cerr << "Failed to load font!" << std::endl;
        }
    }

    void initLayout() {
        // Video area (center-right)
        videoArea.setSize(sf::Vector2f(960, 540));
        videoArea.setPosition(300, 20);
        videoArea.setFillColor(sf::Color::Black);

        // Control panel (bottom)
        controlPanel.setSize(sf::Vector2f(960, 140));
        controlPanel.setPosition(300, 560);
        controlPanel.setFillColor(sf::Color(40, 40, 40));

        // Playlist panel (left)
        playlistPanel.setSize(sf::Vector2f(280, 680));
        playlistPanel.setPosition(10, 20);
        playlistPanel.setFillColor(sf::Color(50, 50, 50));

        // Setup playlist view
        playlistView.reset(sf::FloatRect(10, 20, 280, 680));
        playlistBackground = playlistPanel;
    }

    void initControlButtons() {
        float buttonWidth = 80;
        float buttonHeight = 40;
        float buttonY = 580;
        float spacing = 20;

        // Play/Pause button
        playPauseButton.shape.setSize(sf::Vector2f(buttonWidth, buttonHeight));
        playPauseButton.shape.setPosition(340, buttonY);
        playPauseButton.shape.setFillColor(sf::Color(70, 70, 70));
        playPauseButton.text.setFont(font);
        playPauseButton.text.setString("Play");
        playPauseButton.text.setCharacterSize(18);
        playPauseButton.text.setFillColor(sf::Color::White);
        playPauseButton.text.setPosition(355, buttonY + 10);
        playPauseButton.onClick = [this]() {
            if (isPlaying) {
                mediaManager.pause();
                playPauseButton.text.setString("Play");
            } else {
                mediaManager.play();
                playPauseButton.text.setString("Pause");
            }
            isPlaying = !isPlaying;
        };

        // Stop button

        stopButton.shape.setSize(sf::Vector2f(buttonWidth, buttonHeight));
        stopButton.shape.setPosition(340 + buttonWidth + spacing, buttonY);
        stopButton.shape.setFillColor(sf::Color(70, 70, 70));
        stopButton.text.setFont(font);
        stopButton.text.setString("Stop");
        stopButton.text.setCharacterSize(18);
        stopButton.text.setFillColor(sf::Color::White);
        stopButton.text.setPosition(355 + buttonWidth + spacing, buttonY + 10);
        stopButton.onClick = [this]() {
            mediaManager.stop();
            isPlaying = false;
            playPauseButton.text.setString("Play");
        };

        // Previous button
        prevButton.shape.setSize(sf::Vector2f(buttonWidth, buttonHeight));
        prevButton.shape.setPosition(340 + (buttonWidth + spacing) * 2, buttonY);
        prevButton.shape.setFillColor(sf::Color(70, 70, 70));
        prevButton.text.setFont(font);
        prevButton.text.setString("Prev");
        prevButton.text.setCharacterSize(18);
        prevButton.text.setFillColor(sf::Color::White);
        prevButton.text.setPosition(355 + (buttonWidth + spacing) * 2, buttonY + 10);
        prevButton.onClick = [this]() {
            if (mediaManager.playPrevious()) {
                isPlaying = true;
                playPauseButton.text.setString("Pause");
                updatePlaylistSelection();
            }
        };

        // Next button
        nextButton.shape.setSize(sf::Vector2f(buttonWidth, buttonHeight));
        nextButton.shape.setPosition(340 + (buttonWidth + spacing) * 3, buttonY);
        nextButton.shape.setFillColor(sf::Color(70, 70, 70));
        nextButton.text.setFont(font);
        nextButton.text.setString("Next");
        nextButton.text.setCharacterSize(18);
        nextButton.text.setFillColor(sf::Color::White);
        nextButton.text.setPosition(355 + (buttonWidth + spacing) * 3, buttonY + 10);
        nextButton.onClick = [this]() {
            if (mediaManager.playNext()) {
                isPlaying = true;
                playPauseButton.text.setString("Pause");
                updatePlaylistSelection();
            }
        };

        // Mute button
        muteButton.shape.setSize(sf::Vector2f(buttonWidth, buttonHeight));
        muteButton.shape.setPosition(340 + (buttonWidth + spacing) * 4, buttonY);
        muteButton.shape.setFillColor(sf::Color(70, 70, 70));
        muteButton.text.setFont(font);
        muteButton.text.setString("Mute");
        muteButton.text.setCharacterSize(18);
        muteButton.text.setFillColor(sf::Color::White);
        muteButton.text.setPosition(355 + (buttonWidth + spacing) * 4, buttonY + 10);
        muteButton.onClick = [this]() {
            mediaManager.toggleMute();
            if (mediaManager.isMuted()) {
                muteButton.text.setString("Unmute");
            } else {
                muteButton.text.setString("Mute");
            }
        };

        // Add file button (in playlist area)
        addFileButton.shape.setSize(sf::Vector2f(260, buttonHeight));
        addFileButton.shape.setPosition(20, 30);
        addFileButton.shape.setFillColor(sf::Color(70, 70, 70));
        addFileButton.text.setFont(font);
        addFileButton.text.setString("Add Files");
        addFileButton.text.setCharacterSize(18);
        addFileButton.text.setFillColor(sf::Color::White);
        addFileButton.text.setPosition(110, 40);
        addFileButton.onClick = [this]() {
            showFileDialog = true;
            currentDirectory = "."; // Start in current directory
            updateFileDialog();
        };
    }

    void initSliders() {
        // Timeline slider
        timelineSlider.track.setSize(sf::Vector2f(900, 10));
        timelineSlider.track.setPosition(330, 640);
        timelineSlider.track.setFillColor(sf::Color(100, 100, 100));

        timelineSlider.thumb.setSize(sf::Vector2f(10, 20));
        timelineSlider.thumb.setPosition(330, 635);
        timelineSlider.thumb.setFillColor(sf::Color(200, 200, 200));
        timelineSlider.onChange = [this](float value) {
            if (currentMediaDuration > 0) {
                mediaManager.seek(value * currentMediaDuration);
            }
        };

        // Volume slider
        volumeSlider.track.setSize(sf::Vector2f(100, 10));
        volumeSlider.track.setPosition(750, 595);
        volumeSlider.track.setFillColor(sf::Color(100, 100, 100));

        volumeSlider.thumb.setSize(sf::Vector2f(10, 20));
        volumeSlider.thumb.setPosition(750 + mediaManager.getVolume() * 100, 590);
        volumeSlider.thumb.setFillColor(sf::Color(200, 200, 200));

        volumeSlider.value = mediaManager.getVolume();

        volumeSlider.onChange = [this](float value) {
            mediaManager.setVolume(value);
        };
    }

    void initFileDialog() {
        fileDialogBackground.setSize(sf::Vector2f(600, 500));
        fileDialogBackground.setPosition(340, 110);
        fileDialogBackground.setFillColor(sf::Color(60, 60, 60));

        fileDialogTitle.setFont(font);
        fileDialogTitle.setString("Select File");
        fileDialogTitle.setCharacterSize(20);
        fileDialogTitle.setFillColor(sf::Color::White);
        fileDialogTitle.setPosition(350, 120);

        fileDialogCloseButton.shape.setSize(sf::Vector2f(80, 30));
        fileDialogCloseButton.shape.setPosition(850, 120);
        fileDialogCloseButton.shape.setFillColor(sf::Color(100, 100, 100));
        fileDialogCloseButton.text.setFont(font);
        fileDialogCloseButton.text.setString("Close");
        fileDialogCloseButton.text.setCharacterSize(16);
        fileDialogCloseButton.text.setFillColor(sf::Color::White);
        fileDialogCloseButton.text.setPosition(865, 125);
        fileDialogCloseButton.onClick = [this]() {
            showFileDialog = false;
        };
    }

    void updateFileDialog() {
        fileDialogItems.clear();

        // Get files in current directory
        auto files = mediaManager.listDirectory(currentDirectory);

        // Add parent directory option
        Button parentDir;
        parentDir.shape.setSize(sf::Vector2f(560, 30));
        parentDir.shape.setPosition(360, 160);
        parentDir.shape.setFillColor(sf::Color(80, 80, 80));
        parentDir.text.setFont(font);
        parentDir.text.setString("../");
        parentDir.text.setCharacterSize(16);
        parentDir.text.setFillColor(sf::Color::White);
        parentDir.text.setPosition(370, 165);
        parentDir.onClick = [this]() {
            // Navigate to parent directory
            size_t lastSlash = currentDirectory.find_last_of("/\\");
            if (lastSlash != std::string::npos) {
                currentDirectory = currentDirectory.substr(0, lastSlash);
                if (currentDirectory.empty()) {
                    currentDirectory = ".";
                }
                updateFileDialog();
            }
        };
        fileDialogItems.push_back(parentDir);

        // Add all files and directories
        for (size_t i = 0; i < files.size(); i++) {
            Button fileItem;
            fileItem.shape.setSize(sf::Vector2f(560, 30));
            fileItem.shape.setPosition(360, 160 + (i + 1) * 35);
            fileItem.shape.setFillColor(sf::Color(80, 80, 80));
            fileItem.text.setFont(font);
            fileItem.text.setString(files[i].isDirectory ? "[DIR] " + files[i].name : files[i].name);
            fileItem.text.setCharacterSize(16);
            fileItem.text.setFillColor(sf::Color::White);
            fileItem.text.setPosition(370, 165 + (i + 1) * 35);

            // Copy the file info for the lambda
            auto fileInfo = files[i];
            fileItem.onClick = [this, fileInfo]() {
                if (fileInfo.isDirectory) {
                    // Navigate to directory
                    currentDirectory += "/" + fileInfo.name;
                    updateFileDialog();
                } else {
                    // Select file
                    std::string fullPath = currentDirectory + "/" + fileInfo.name;
                    mediaManager.addToPlaylist(fullPath);
                    updatePlaylist();
                    showFileDialog = false;
                }
            };

            fileDialogItems.push_back(fileItem);
        }
    }

    void updatePlaylist() {
        playlistItems.clear();

        float itemHeight = 30;
        float spacing = 5;
        float startY = 80; // Position after "Add Files" button

        // Display all items in the current playlist
        auto& playlistManager = mediaManager.getPlaylistManager();
        auto playlist = playlistManager.getCurrentPlaylist();

        for (size_t i = 0; i < playlist->getItemCount(); i++) {
            Button item;
            item.shape.setSize(sf::Vector2f(260, itemHeight));
            item.shape.setPosition(20, startY + i * (itemHeight + spacing));

            // Highlight the current playing item
            if (mediaManager.getCurrentFilePath() == playlist->getItem(i).path) {
                item.shape.setFillColor(sf::Color(100, 100, 150));
            } else {
                item.shape.setFillColor(sf::Color(70, 70, 70));
            }

            // Extract filename from path
            std::string filename = playlist->getItem(i).path;
            size_t lastSlash = filename.find_last_of("/\\");
            if (lastSlash != std::string::npos) {
                filename = filename.substr(lastSlash + 1);
            }

            item.text.setFont(font);
            item.text.setString(filename);
            item.text.setCharacterSize(14);
            item.text.setFillColor(sf::Color::White);

            // Truncate long filenames
            if (filename.length() > 30) {
                item.text.setString(filename.substr(0, 27) + "...");
            }

            item.text.setPosition(25, startY + i * (itemHeight + spacing) + 5);

            // Copy index for the lambda
            size_t index = i;
            item.onClick = [this, index]() {
                if (mediaManager.openPlaylistItem(index)) {
                    isPlaying = true;
                    playPauseButton.text.setString("Pause");
                    updatePlaylistSelection();
                }
            };

            playlistItems.push_back(item);
        }

    }

    void updatePlaylistSelection() {
        updatePlaylist(); // Refresh playlist to highlight current item
    }

    void setupMediaCallbacks() {
        mediaManager.setPlaybackStartCallback([this]() {
            isPlaying = true;
            playPauseButton.text.setString("Pause");
            currentMediaDuration = mediaManager.getDuration();
            updatePlaylistSelection();
        });

        mediaManager.setPlaybackPauseCallback([this]() {
            isPlaying = false;
            playPauseButton.text.setString("Play");
        });

        mediaManager.setPlaybackStopCallback([this]() {
            isPlaying = false;
            playPauseButton.text.setString("Play");
        });

        mediaManager.setPlaybackEndCallback([this]() {
            // If not repeat mode, go to next when playback ends
            if (mediaManager.getRepeatMode() == 0) {
                mediaManager.playNext();
            }
        });

        mediaManager.setPositionChangeCallback([this](double position) {
            if (!userIsSeeking && currentMediaDuration > 0) {
                float normalized = static_cast<float>(position / currentMediaDuration);
                timelineSlider.value = normalized;
                timelineSlider.thumb.setPosition(
                    330 + normalized * 900,
                    635
                );
            }
        });

        mediaManager.setVolumeChangeCallback([this](float volume) {
            volumeSlider.value = volume;
            volumeSlider.thumb.setPosition(
                750 + volume * 100,
                590
            );
        });

        mediaManager.setFrameReadyCallback([this]() {
            // Update the video texture with the new frame
            updateVideoTexture();
        });
    }

    void updateVideoTexture() {
        // This would actually get the video frame from MediaPlayer
        // In a real implementation, this would involve getting the raw frame data
        // and updating the SFML texture with it

        // For this example, just show a placeholder
        videoTexture.clear(sf::Color::Black);

        if (isPlaying) {
            // In a real implementation, you would draw the actual video frame here
            sf::Text text;
            text.setFont(font);
            text.setString("Playing: " + mediaManager.getCurrentFileName());
            text.setCharacterSize(24);
            text.setFillColor(sf::Color::White);
            text.setPosition(50, 50);
            videoTexture.draw(text);

            // Add current time/duration
            sf::Text timeText;
            timeText.setFont(font);
            double position = mediaManager.getCurrentPosition();
            double duration = mediaManager.getDuration();

            int posMinutes = static_cast<int>(position) / 60;
            int posSeconds = static_cast<int>(position) % 60;
            int durMinutes = static_cast<int>(duration) / 60;
            int durSeconds = static_cast<int>(duration) % 60;

            std::string timeString = std::to_string(posMinutes) + ":" +
                                    (posSeconds < 10 ? "0" : "") + std::to_string(posSeconds) +
                                    " / " +
                                    std::to_string(durMinutes) + ":" +
                                    (durSeconds < 10 ? "0" : "") + std::to_string(durSeconds);

            timeText.setString(timeString);
            timeText.setCharacterSize(18);
            timeText.setFillColor(sf::Color::White);
            timeText.setPosition(50, 100);
            videoTexture.draw(timeText);
        } else {
            sf::Text text;
            text.setFont(font);
            text.setString("Video Player (Paused)");
            text.setCharacterSize(24);
            text.setFillColor(sf::Color::White);
            text.setPosition(50, 50);
            videoTexture.draw(text);
        }

        videoTexture.display();
        videoSprite.setTexture(videoTexture.getTexture());
        videoSprite.setPosition(videoArea.getPosition());
        videoSprite.setScale(
            videoArea.getSize().x / videoTexture.getSize().x,
            videoArea.getSize().y / videoTexture.getSize().y
        );
    }

    bool isMouseOver(const sf::RectangleShape& shape) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        sf::Vector2f mousePosF(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));
        return shape.getGlobalBounds().contains(mousePosF);
    }

    void handleButtonHover(Button& button) {
        bool hovering = isMouseOver(button.shape);
        if (hovering != button.isHovered) {
            button.isHovered = hovering;
            if (hovering) {
                button.shape.setFillColor(sf::Color(100, 100, 100));
            } else {
                button.shape.setFillColor(sf::Color(70, 70, 70));
            }
        }
    }

    void handlePlaylistScroll(float delta) {
        if (playlistItems.size() > 0) {
            playlistScrollPosition += delta * 20.0f;
            playlistScrollPosition = std::max(0.0f, std::min(playlistScrollPosition,
                                                          static_cast<float>(playlistItems.size() * 35) - 580.0f));
            for (size_t i = 0; i < playlistItems.size(); i++) {
                playlistItems[i].shape.setPosition(20, 80 + i * 35 - playlistScrollPosition);
                playlistItems[i].text.setPosition(25, 85 + i * 35 - playlistScrollPosition);
            }
        }
    }

    public:
    VideoPlayerGUI() : mediaManager(VideoPlayer::Core::MediaPlayerManager::getInstance()) {
        initWindow();
        initFont();
        initLayout();
        initControlButtons();
        initSliders();
        initFileDialog();
        setupMediaCallbacks();
        updatePlaylist();
        updateVideoTexture();
    }

    void run() {
        while (window.isOpen()) {
            handleEvents();
            update();
            render();
        }
    }

    void handleEvents() {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            } else if (event.type == sf::Event::MouseButtonPressed) {
                if (event.mouseButton.button == sf::Mouse::Left) {
                    handleMouseClick(event.mouseButton.x, event.mouseButton.y);
                }
            } else if (event.type == sf::Event::MouseButtonReleased) {
                if (event.mouseButton.button == sf::Mouse::Left) {
                    timelineSlider.isDragging = false;
                    volumeSlider.isDragging = false;
                    userIsSeeking = false;
                }
            } else if (event.type == sf::Event::MouseMoved) {
                handleMouseMove(event.mouseMove.x, event.mouseMove.y);
            } else if (event.type == sf::Event::MouseWheelScrolled) {
                if (isMouseOver(playlistPanel)) {
                    handlePlaylistScroll(event.mouseWheelScroll.delta);
                }
            }
        }
    }

    void handleMouseClick(int x, int y) {
        sf::Vector2f mousePos(static_cast<float>(x), static_cast<float>(y));

        if (showFileDialog) {
            // Handle file dialog clicks
            if (fileDialogCloseButton.shape.getGlobalBounds().contains(mousePos)) {
                fileDialogCloseButton.onClick();
            }

            for (auto& item : fileDialogItems) {
                if (item.shape.getGlobalBounds().contains(mousePos)) {
                    item.onClick();
                    break;
                }
            }
            return;
        }

        // Check control buttons
        if (playPauseButton.shape.getGlobalBounds().contains(mousePos)) {
            playPauseButton.onClick();
        } else if (stopButton.shape.getGlobalBounds().contains(mousePos)) {
            stopButton.onClick();
        } else if (prevButton.shape.getGlobalBounds().contains(mousePos)) {
            prevButton.onClick();
        } else if (nextButton.shape.getGlobalBounds().contains(mousePos)) {
            nextButton.onClick();
        } else if (muteButton.shape.getGlobalBounds().contains(mousePos)) {
            muteButton.onClick();
        } else if (addFileButton.shape.getGlobalBounds().contains(mousePos)) {
            addFileButton.onClick();
        }

        // Check playlist items
        for (auto& item : playlistItems) {
            if (item.shape.getGlobalBounds().contains(mousePos)) {
                item.onClick();
                break;
            }
        }

        // Check sliders
        if (timelineSlider.track.getGlobalBounds().contains(mousePos) ||
            timelineSlider.thumb.getGlobalBounds().contains(mousePos)) {
            timelineSlider.isDragging = true;
            userIsSeeking = true;

            // Update position based on click
            float relativeX = mousePos.x - timelineSlider.track.getPosition().x;
            float width = timelineSlider.track.getSize().x;
            float value = std::max(0.0f, std::min(relativeX / width, 1.0f));
            timelineSlider.value = value;
            timelineSlider.thumb.setPosition(
                timelineSlider.track.getPosition().x + value * width,
                timelineSlider.thumb.getPosition().y
            );
            timelineSlider.onChange(value);
        }

        if (volumeSlider.track.getGlobalBounds().contains(mousePos) ||
        volumeSlider.thumb.getGlobalBounds().contains(mousePos)) {
            volumeSlider.isDragging = true;

            // Update volume based on click
            float relativeX = mousePos.x - volumeSlider.track.getPosition().x;
            float width = volumeSlider.track.getSize().x;
            float value = std::max(0.0f, std::min(relativeX / width, 1.0f));
            volumeSlider.value = value;
            volumeSlider.thumb.setPosition(
                volumeSlider.track.getPosition().x + value * width,
                volumeSlider.thumb.getPosition().y
            );
            volumeSlider.onChange(value);
        }
    }

    void handleMouseMove(int x, int y) {
        sf::Vector2f mousePos(static_cast<float>(x), static_cast<float>(y));

        // Handle button hover states
        handleButtonHover(playPauseButton);
        handleButtonHover(stopButton);
        handleButtonHover(prevButton);
        handleButtonHover(nextButton);
        handleButtonHover(muteButton);
        handleButtonHover(addFileButton);

        for (auto& item : playlistItems) {
            handleButtonHover(item);
        }

        if (showFileDialog) {
            handleButtonHover(fileDialogCloseButton);
            for (auto& item : fileDialogItems) {
                handleButtonHover(item);
            }
        }

        // Handle slider dragging
        if (timelineSlider.isDragging) {
            float relativeX = mousePos.x - timelineSlider.track.getPosition().x;
            float width = timelineSlider.track.getSize().x;
            float value = std::max(0.0f, std::min(relativeX / width, 1.0f));
            timelineSlider.value = value;
            timelineSlider.thumb.setPosition(
                timelineSlider.track.getPosition().x + value * width,
                timelineSlider.thumb.getPosition().y
            );
            timelineSlider.onChange(value);
        }

        if (volumeSlider.isDragging) {
            float relativeX = mousePos.x - volumeSlider.track.getPosition().x;
            float width = volumeSlider.track.getSize().x;
            float value = std::max(0.0f, std::min(relativeX / width, 1.0f));
            volumeSlider.value = value;
            volumeSlider.thumb.setPosition(
                volumeSlider.track.getPosition().x + value * width,
                volumeSlider.thumb.getPosition().y
            );
            volumeSlider.onChange(value);
        }
    }

    void update() {
        // Regular updates (30 times per second)
        if (clock.getElapsedTime().asMilliseconds() > 33) {
            clock.restart();

            if (isPlaying) {
                updateVideoTexture();

                // Update the time display
                if (!userIsSeeking) {
                    double position = mediaManager.getCurrentPosition();
                    double duration = mediaManager.getDuration();
                    if (duration > 0) {
                        float normalized = static_cast<float>(position / duration);
                        timelineSlider.value = normalized;
                        timelineSlider.thumb.setPosition(
                            timelineSlider.track.getPosition().x + normalized * timelineSlider.track.getSize().x,
                            timelineSlider.thumb.getPosition().y
                        );
                    }
                }
            }
        }
    }

    void render() {
        window.clear(sf::Color(30, 30, 30));

        // Draw layout panels
        window.draw(videoArea);
        window.draw(controlPanel);
        window.draw(playlistPanel);

        // Draw video content
        window.draw(videoSprite);

        // Draw control buttons
        window.draw(playPauseButton.shape);
        window.draw(playPauseButton.text);
        window.draw(stopButton.shape);
        window.draw(stopButton.text);
        window.draw(prevButton.shape);
        window.draw(prevButton.text);
        window.draw(nextButton.shape);
        window.draw(nextButton.text);
        window.draw(muteButton.shape);
        window.draw(muteButton.text);
        window.draw(addFileButton.shape);
        window.draw(addFileButton.text);

        // Draw sliders
        window.draw(timelineSlider.track);
        window.draw(timelineSlider.thumb);
        window.draw(volumeSlider.track);
        window.draw(volumeSlider.thumb);

        // Draw timeline labels
        sf::Text currentTime;
        currentTime.setFont(font);
        currentTime.setString("00:00");
        currentTime.setCharacterSize(14);
        currentTime.setFillColor(sf::Color::White);
        currentTime.setPosition(330, 655);
        window.draw(currentTime);

        sf::Text totalTime;
        totalTime.setFont(font);
        if (currentMediaDuration > 0) {
            int minutes = static_cast<int>(currentMediaDuration) / 60;
            int seconds = static_cast<int>(currentMediaDuration) % 60;
            totalTime.setString(std::to_string(minutes) + ":" + (seconds < 10 ? "0" : "") + std::to_string(seconds));
        } else {
            totalTime.setString("00:00");
        }
        totalTime.setCharacterSize(14);
        totalTime.setFillColor(sf::Color::White);
        totalTime.setPosition(1200, 655);
        window.draw(totalTime);

        // Draw volume label
        sf::Text volumeLabel;
        volumeLabel.setFont(font);
        volumeLabel.setString("Volume");
        volumeLabel.setCharacterSize(14);
        volumeLabel.setFillColor(sf::Color::White);
        volumeLabel.setPosition(700, 595);
        window.draw(volumeLabel);

        // Draw playlist items
        for (const auto& item : playlistItems) {
            if (item.shape.getPosition().y + item.shape.getSize().y >= 80 &&
                item.shape.getPosition().y <= 680) {
                window.draw(item.shape);
                window.draw(item.text);
            }
        }

        // Draw file dialog if active
        if (showFileDialog) {
            window.draw(fileDialogBackground);
            window.draw(fileDialogTitle);
            window.draw(fileDialogCloseButton.shape);
            window.draw(fileDialogCloseButton.text);

            for (const auto& item : fileDialogItems) {
                window.draw(item.shape);
                window.draw(item.text);
            }
        }

        window.display();
    }
};

int main() {
    // Initialize MediaPlayerManager
    VideoPlayer::Core::MediaPlayerManager& mediaManager =
        VideoPlayer::Core::MediaPlayerManager::getInstance();

    if (!mediaManager.initialize()) {
        std::cerr << "Failed to initialize MediaPlayerManager!" << std::endl;
        return -1;
    }

    // Create and run the GUI
    VideoPlayerGUI gui;
    gui.run();

    // Cleanup
    mediaManager.shutdown();

    return 0;
}
