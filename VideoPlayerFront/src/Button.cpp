#pragma once

#include <iostream>
#include "../include/Button.hpp"

using namespace Colors;

Button::Button(const std::string& label, const sf::Font& font) {
    shape.setSize(sf::Vector2f(80, 30));
    shape.setFillColor(BUTTON_COLOR);

    text.setFont(font);
    text.setString(label);
    text.setCharacterSize(14);
    text.setFillColor(TEXT_COLOR);
}

void Button::setPosition(float x, float y) {
    shape.setPosition(x, y);
    centerText();
}

void Button::draw(sf::RenderWindow& window) {
    window.draw(shape);
    window.draw(text);
}

bool Button::contains(const sf::Vector2f& point) const {
    return shape.getGlobalBounds().contains(point);
}

void Button::setHoverState(bool isHovering) {
    shape.setFillColor(isHovering ? BUTTON_HOVER_COLOR : BUTTON_COLOR);
}

void Button::setActiveState(bool isActive) {
    shape.setFillColor(isActive ? BUTTON_ACTIVE_COLOR : BUTTON_COLOR);
}

void Button::centerText() {
    sf::FloatRect textBounds = text.getLocalBounds();
    text.setPosition(shape.getPosition().x + (shape.getSize().x - textBounds.width) / 2.0f - textBounds.left,
                     shape.getPosition().y + (shape.getSize().y - textBounds.height) / 2.0f - textBounds.top - 2.0f);
}
