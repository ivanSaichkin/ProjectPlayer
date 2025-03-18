#ifndef SCROLLBAR_HPP
#define SCROLLBAR_HPP

#include <SFML/Graphics.hpp>
#include <functional>
#include "Button.hpp"

class ScrollBar : public Button {
public:
    ScrollBar(const sf::Vector2f& position, const sf::Vector2f& size, const sf::Color& trackColor, const sf::Color& thumbColor);

    void draw(sf::RenderWindow& window) const override;
    void updateThumbPosition(float progress); // Обновляет позицию бегунка в зависимости от прогресса (0.0 - 1.0)
    void onClick(std::function<void(float)> callback); // Callback для обработки клика на полосе прокрутки

    bool isMouseOver(const sf::RenderWindow& window) const;

private:
    sf::RectangleShape track; // Фон полосы прокрутки
    sf::CircleShape thumb;    // Бегунок (теперь круглый)
    std::function<void(float)> onClickCallback; // Callback для обработки клика (теперь с аргументом float)

    void updateThumb(); // Обновляет позицию и размер бегунка
};

#endif // SCROLLBAR_HPP
