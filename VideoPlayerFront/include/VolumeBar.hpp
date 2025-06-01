#pragma once

#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include "../include/Button.hpp"


class VolumeBar {
    public:
        VolumeBar(const sf::Font& font);

        void setSize(float width, float height);
        void setPosition(float x, float y);
        void update(float newVolume);

        void draw(sf::RenderWindow& window);
        bool contains(const sf::Vector2f& point) const;

        float getVolumeFromClick(float mouseX) const;
        float getVolume() const;

    private:
        void updateDisplay();
        void updateHandlePosition();

        sf::RectangleShape background;
        sf::RectangleShape fill;
        sf::RectangleShape handle;
        sf::Text volumeText;
        float volume;
        float x, y;
    };
