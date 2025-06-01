#pragma once

#include <iostream>
#include "../include/ProgressBar.hpp"

using namespace Colors;

ProgressBar::ProgressBar(const sf::Font& font) {
    background.setFillColor(PROGRESS_BAR_BG_COLOR);
    fill.setFillColor(PROGRESS_BAR_FILL_COLOR);

    timeText.setFont(font);
    timeText.setCharacterSize(12);
    timeText.setFillColor(TEXT_COLOR);
    timeText.setString("00:00 / 00:00");
}

void ProgressBar::setSize(float width, float height) {
    background.setSize(sf::Vector2f(width, height));
}

void ProgressBar::setPosition(float x, float y) {
    background.setPosition(x, y);
    fill.setPosition(x, y);
    timeText.setPosition(x, y + background.getSize().y + 5.0f);
}

void ProgressBar::update(double currentTime, double duration, bool isPlaying) {
    this->currentTime = currentTime;
    this->duration = duration;

    // Update fill width based on current position
    float fillWidth = (duration > 0) ? (currentTime / duration) * background.getSize().x : 0;
    fill.setSize(sf::Vector2f(fillWidth, background.getSize().y));

    // Update time text
    timeText.setString(formatTime(currentTime) + " / " + formatTime(duration));
}

bool ProgressBar::isVideoEnded() const {
    return duration > 0 && currentTime >= duration;
}

void ProgressBar::draw(sf::RenderWindow& window) {
    window.draw(background);
    window.draw(fill);
    window.draw(timeText);
}

bool ProgressBar::contains(const sf::Vector2f& point) const {
    return background.getGlobalBounds().contains(point);
}

double ProgressBar::getPositionFromClick(float x) const {
    float relativeX = std::max(0.0f, std::min(x - background.getPosition().x, background.getSize().x));
    float ratio = relativeX / background.getSize().x;
    return ratio * duration;
}

std::string ProgressBar::formatTime(double seconds) {
    int mins = static_cast<int>(seconds) / 60;
    int secs = static_cast<int>(seconds) % 60;
    return std::to_string(mins) + ":" + (secs < 10 ? "0" : "") + std::to_string(secs);
}
