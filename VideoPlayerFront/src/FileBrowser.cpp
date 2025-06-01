#pragma once

#include <iostream>
#include "../include/FileBrowser.hpp"

using namespace Colors;

FileBrowser::FileBrowser(const sf::Font& font) : font(font) {
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

void FileBrowser::setSize(float width, float height) {
    this->width = width;
    this->height = height;
    background.setSize(sf::Vector2f(width, height));

    closeButton->setPosition(width - 40, 20);
    openButton->setPosition(width - 180, height - 50);
    cancelButton->setPosition(width - 90, height - 50);

    fileListHeight = height - 120;  // Высота области списка файлов
}

void FileBrowser::setPosition(float x, float y) {
    this->x = x;
    this->y = y;
    background.setPosition(x, y);
    title.setPosition(x + 20, y + 20);

    closeButton->setPosition(x + width - 40, y + 20);
    openButton->setPosition(x + width - 180, y + height - 50);
    cancelButton->setPosition(x + width - 90, y + height - 50);
}

void FileBrowser::show() {
    isActive = true;
    selectedIndex = -1;
    scrollOffset = 0;
    loadCurrentDirectory();
}

void FileBrowser::draw(sf::RenderWindow& window) {
    if (!isActive)
        return;

    window.draw(background);
    window.draw(title);

    // Рисуем список файлов
    drawFileList(window);

    closeButton->draw(window);
    openButton->draw(window);
    cancelButton->draw(window);
}

void FileBrowser::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    if (!isActive)
        return;

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
        if (event.mouseWheelScroll.x >= x && event.mouseWheelScroll.x <= x + width && event.mouseWheelScroll.y >= y + 60 &&
            event.mouseWheelScroll.y <= y + 60 + fileListHeight) {
            scrollOffset -= event.mouseWheelScroll.delta * 3;
            scrollOffset = std::max(0, std::min(scrollOffset, static_cast<int>(currentFiles.size()) - getVisibleFileCount()));
        }
    }
}

bool FileBrowser::isVisible() const {
    return isActive;
}

void FileBrowser::setFileSelectedCallback(std::function<void(const std::string&)> callback) {
    onFileSelected = callback;
}

void FileBrowser::loadCurrentDirectory() {
    currentFiles.clear();
    try {
        // Загружаем файлы из указанной директории
        std::string targetPath = "../Test";

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

void FileBrowser::drawFileList(sf::RenderWindow& window) {
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

void FileBrowser::handleFileListClick(const sf::Vector2f& mousePos) {
    const float itemHeight = 25.0f;
    const float listY = y + 60;

    if (mousePos.x >= x + 20 && mousePos.x <= x + width - 20 && mousePos.y >= listY && mousePos.y <= listY + fileListHeight) {
        int clickedIndex = static_cast<int>((mousePos.y - listY) / itemHeight) + scrollOffset;
        if (clickedIndex >= 0 && clickedIndex < currentFiles.size()) {
            selectedIndex = clickedIndex;
        }
    }
}

int FileBrowser::getVisibleFileCount() const {
    return static_cast<int>(fileListHeight / 25.0f);
}
