#pragma once

#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <string>
#include <cmath>
#include <filesystem>
#include <vector>
#include <algorithm>

namespace Colors {
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
}

class Button {
    public:
        Button(const std::string& label, const sf::Font& font);

        void setPosition(float x, float y);
        void draw(sf::RenderWindow& window);
        bool contains(const sf::Vector2f& point) const;

        void setHoverState(bool isHovering);
        void setActiveState(bool isActive);
    private:
        void centerText();
        void updateAnimation();

    sf::RectangleShape shape;
    sf::Text text;
    sf::Vector2f originalPosition;
    bool isPressed;
    sf::Clock animationClock;
    float animationDuration;
    };
