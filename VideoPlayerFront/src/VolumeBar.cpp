#pragma once

#include <iostream>
#include "../include/VolumeBar.hpp"

using namespace Colors;

VolumeBar::VolumeBar(const sf::Font& font) {
    background.setFillColor(VOLUME_BAR_BG_COLOR);
    fill.setFillColor(VOLUME_BAR_FILL_COLOR);
    handle.setFillColor(sf::Color::White);

    volumeText.setFont(font);
    volumeText.setCharacterSize(12);
    volumeText.setFillColor(TEXT_COLOR);

    volume = 100.0f;  // Default volume 100%
    updateDisplay();
}

void VolumeBar::setSize(float width, float height) {
    background.setSize(sf::Vector2f(width, height));
    handle.setSize(sf::Vector2f(8, height + 4));
}

void VolumeBar::setPosition(float x, float y) {
    this->x = x;
    this->y = y;
    background.setPosition(x, y);
    fill.setPosition(x, y);
    volumeText.setPosition(x, y - 20);
    updateHandlePosition();
}

void VolumeBar::update(float newVolume) {
    volume = std::max(0.0f, std::min(100.0f, newVolume));
    updateDisplay();
}

void VolumeBar::draw(sf::RenderWindow& window) {
    window.draw(background);
    window.draw(fill);
    window.draw(handle);
    window.draw(volumeText);
}

bool VolumeBar::contains(const sf::Vector2f& point) const {
    sf::FloatRect bounds = background.getGlobalBounds();
    bounds.height += 8;  // Увеличить область для удобства
    bounds.top -= 4;
    return bounds.contains(point);
}

float VolumeBar::getVolumeFromClick(float mouseX) const {
    float relativeX = mouseX - x;
    float ratio = relativeX / background.getSize().x;
    return std::max(0.0f, std::min(1.0f, ratio)) * 100.0f;
}

float VolumeBar::getVolume() const {
    return volume;
}

void VolumeBar::updateDisplay() {
    float fillWidth = (volume / 100.0f) * background.getSize().x;
    fill.setSize(sf::Vector2f(fillWidth, background.getSize().y));

    volumeText.setString("Volume: " + std::to_string(static_cast<int>(volume)) + "%");
    updateHandlePosition();
}

void VolumeBar::updateHandlePosition() {
    float handleX = x + (volume / 100.0f) * background.getSize().x - 4;
    handle.setPosition(handleX, y - 2);
}
