#pragma once

#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include "../include/Button.hpp"

class FileBrowser {
    public:
        FileBrowser(const sf::Font& font);

        void setSize(float width, float height);
        void setPosition(float x, float y);

        void show();
        void draw(sf::RenderWindow& window);
        void handleEvent(const sf::Event& event, sf::RenderWindow& window);
        bool isVisible() const;
        void setFileSelectedCallback(std::function<void(const std::string&)> callback);

    private:
        void loadCurrentDirectory();
        void drawFileList(sf::RenderWindow& window);
        void handleFileListClick(const sf::Vector2f& mousePos);
        int getVisibleFileCount() const;

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
