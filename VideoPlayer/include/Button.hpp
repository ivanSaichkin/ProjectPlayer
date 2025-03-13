#ifndef BUTTON_HPP
#define BUTTON_HPP

#include <SFML/Graphics.hpp>
#include <functional>

class Button {
public:
    Button(const std::string& label, const sf::Font& font, const sf::Vector2f& position, const sf::Vector2f& size);
    Button();

    void draw(sf::RenderWindow& window) const; // возможно лишнее/надо изменить
    bool isMouseOver(const sf::RenderWindow& window) const;
    void onClick(std::function<void()> callback);

    void setLabel(const std::string& label);
    void setPosition(const sf::Vector2f& position);
    void setSize(const sf::Vector2f& size);

private:
    sf::RectangleShape shape;
    sf::Text labelText;
    std::function<void()> onClickCallback;

    void updateTextPosition();
};

#endif // BUTTON_HPP
