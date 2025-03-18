#include "/Users/andreypavlinich/a/PlayerRep/ProjectPlayer/VideoPlayer/include/Button.hpp"
#include "/Users/andreypavlinich/a/PlayerRep/ProjectPlayer/VideoPlayer/include/ScrollBar.hpp"
#include <iostream>


ScrollBar::ScrollBar(const sf::Vector2f& position, const sf::Vector2f& size, const sf::Color& trackColor, const sf::Color& thumbColor)
    : Button() {
    // Инициализация фона (track)
    track.setSize(size);
    track.setPosition(position);
    track.setFillColor(trackColor);

    // Инициализация бегунка (thumb)
    thumb.setRadius(size.y / 2.0f); // Бегунок будет круглым с радиусом, равным половине высоты полосы
    thumb.setFillColor(thumbColor);
    thumb.setOrigin(thumb.getRadius(), thumb.getRadius()); // Центрируем бегунок

    // Устанавливаем начальную позицию бегунка
    updateThumbPosition(0.0f);

    // Устанавливаем размер и позицию для базового класса Button
    setPosition(position);
    setSize(size);
}

void ScrollBar::draw(sf::RenderWindow& window) const {
    window.draw(track); // Рисуем фон
    window.draw(thumb); // Рисуем бегунок
}

void ScrollBar::updateThumbPosition(float progress) {
    // Ограничиваем прогресс в пределах [0.0, 1.0]
    progress = std::max(0.0f, std::min(1.0f, progress));

    // Вычисляем новую позицию бегунка
    float thumbX = track.getPosition().x + (track.getSize().x - 2 * thumb.getRadius()) * progress;
    float thumbY = track.getPosition().y + track.getSize().y / 2.0f; // Центрируем по вертикали
    thumb.setPosition(thumbX, thumbY);

    // Вызываем callback, если он установлен
    if (onClickCallback) {
        onClickCallback(progress);
    }
}

void ScrollBar::onClick(std::function<void(float)> callback) {
    onClickCallback = callback; // Сохраняем callback
}

bool ScrollBar::isMouseOver(const sf::RenderWindow& window) const {
    sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    // Проверяем, находится ли курсор над бегунком (кругом)
    float distanceSquared = std::pow(mousePos.x - thumb.getPosition().x, 2) + std::pow(mousePos.y - thumb.getPosition().y, 2);
    return distanceSquared <= std::pow(thumb.getRadius(), 2);
}
