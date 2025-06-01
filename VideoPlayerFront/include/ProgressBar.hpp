#pragma once

#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include "../include/Button.hpp"

class ProgressBar {
    public:
        ProgressBar(const sf::Font& font);

        void setSize(float width, float height);
        void setPosition(float x, float y);

        void update(double currentTime, double duration, bool isPlaying);
        bool isVideoEnded() const;
        void draw(sf::RenderWindow& window);
        bool contains(const sf::Vector2f& point) const;
        double getPositionFromClick(float x) const;

    private:
        std::string formatTime(double seconds);

        sf::RectangleShape background;
        sf::RectangleShape fill;
        sf::Text timeText;
        double currentTime = 0.0;
        double duration = 0.0;
    };
