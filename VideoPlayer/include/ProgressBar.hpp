#ifndef SCROLLBAR_HPP
#define SCROLLBAR_HPP

#include <SFML/Graphics.hpp>
#include <functional>
#include "Button.hpp"

class ProgressBar : public Button {
public:
    ProgressBar(const sf::Vector2f& position, const sf::Vector2f& size, const sf::Color& trackColor, const sf::Color& thumbColor);

    void draw(sf::RenderWindow& window) const override;
    void onClick(std::function<void(float)> callback); // Callback для обработки клика на полосе прокрутки

    bool isMouseOverThumb(const sf::RenderWindow& window) const;
    bool isMouseOverTrack(const sf::RenderWindow& window) const;

    void setThumbColor(const sf::Color& color);
    void setTrackColor(const sf::Color& color);

    const sf::CircleShape& getThumb() const;
    const sf::RectangleShape& getTrack() const;

    void changePosition(const sf::Vector2f& position); // для нажатой лкм (мб поменять название)
    bool isThumbClicked(const sf::RenderWindow& window) const; // Проверяет, нажат ли ползунок
    bool isTrackClicked(const sf::RenderWindow& window) const; // нажат ли трек прогрессбара
    void updateThumbFromMouse(const sf::RenderWindow& window);
    void updateThumbPosition(const sf::Vector2f& position); // Обновляет позицию бегунка в зависимости от прогресса (0.0 - 1.0)

    void startDragging();
    void stopDragging();
    bool getIsDragging() const; // nahui nado


private:
    sf::RectangleShape track; // Фон полосы прокрутки
    sf::CircleShape thumb;    // Бегунок (теперь круглый)
    std::function<void(float)> onClickCallback; // Callback для обработки клика (теперь с аргументом float)
    float progress = 0.f;
    bool isDragging = false;

    void updateThumb(); // Обновляет позицию и размер бегунка
    float calculateProgress(const sf::Vector2f& mousePos) const;
};

#endif // SCROLLBAR_HPP
