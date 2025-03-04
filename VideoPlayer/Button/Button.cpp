#include "Button.hpp"
#include <iostream>

Button::Button(const std::string& label, const sf::Font& font, const sf::Vector2f& position, const sf::Vector2f& size) {
    shape.setSize(size);
    shape.setPosition(position);
    shape.setFillColor(sf::Color::Green);

    labelText.setFont(font);
    labelText.setString(label);
    labelText.setCharacterSize(20);
    labelText.setFillColor(sf::Color::White);

    updateTextPosition();
}

void Button::draw(sf::RenderWindow& window) const {
    window.draw(shape);
    window.draw(labelText);
}

bool Button::isMouseOver(const sf::RenderWindow& window) const {
    sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    return shape.getGlobalBounds().contains(mousePos);
}

void Button::onClick(std::function<void()> callback) {
    onClickCallback = callback;
}

void Button::setLabel(const std::string& label) {
    labelText.setString(label);
    updateTextPosition();
}

void Button::setPosition(const sf::Vector2f& position) {
    shape.setPosition(position);
    updateTextPosition();
}

void Button::setSize(const sf::Vector2f& size) {
    shape.setSize(size);
    updateTextPosition();
}

void Button::updateTextPosition() {
    sf::FloatRect textRect = labelText.getLocalBounds();
    labelText.setOrigin(textRect.left + textRect.width / 2.0f, textRect.top + textRect.height / 2.0f);
    labelText.setPosition(shape.getPosition().x + shape.getSize().x / 2.0f, shape.getPosition().y + shape.getSize().y / 2.0f);
}
