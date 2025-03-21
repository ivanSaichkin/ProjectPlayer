#include "/Users/andreypavlinich/a/PlayerRep/ProjectPlayer/VideoPlayer/include/Button.hpp"
#include "/Users/andreypavlinich/a/PlayerRep/ProjectPlayer/VideoPlayer/include/ProgressBar.hpp"
#include <iostream>


ProgressBar::ProgressBar(const sf::Vector2f& position, const sf::Vector2f& size, const sf::Color& trackColor, const sf::Color& thumbColor)
    : Button() {
    // Инициализация фона (track)
    track.setSize(size);
    track.setPosition(position);
    track.setFillColor(trackColor);

    // Инициализация бегунка (thumb)
    thumb.setRadius(size.y); // Бегунок будет круглым с радиусом
    thumb.setFillColor(thumbColor);
    thumb.setOrigin(thumb.getRadius(), thumb.getRadius()); // Центрируем бегунок

    // Устанавливаем начальную позицию бегунка
    updateThumbPosition(0.0f);

    // Устанавливаем размер и позицию для базового класса Button
    setPosition(position);
    setSize(size);
}

void ProgressBar::draw(sf::RenderWindow& window) const {
    window.draw(track); // Рисуем фон
    window.draw(thumb); // Рисуем бегунок
}

// здесь надо связывать с декодированием уже
void ProgressBar::updateThumbPosition(float progress) {
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

void ProgressBar::onClick(std::function<void(float)> callback) {
    onClickCallback = callback; // Сохраняем callback (а надо ли его ваще использовать??)
}

bool ProgressBar::isMouseOver(const sf::RenderWindow& window) const {
    sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    // Проверяем, находится ли курсор над бегунком (кругом)
    float distanceSquared = std::pow(mousePos.x - thumb.getPosition().x, 2) + std::pow(mousePos.y - thumb.getPosition().y, 2);
    return distanceSquared <= std::pow(thumb.getRadius(), 2);
}

void ProgressBar::changePosition(const sf::Vector2f& position) {
    this->setPosition(position);

}

void ProgressBar::setThumbColor(const sf::Color& color) {
    thumb.setFillColor(color); // Устанавливаем цвет ползунка
}

void ProgressBar::setTrackColor(const sf::Color& color) {
    track.setFillColor(color);
}

bool ProgressBar::isThumbClicked(const sf::RenderWindow& window) const {
    // Проверяем, находится ли курсор мыши над бегунком
    if (isMouseOver(window)) {
        // Проверяем, нажата ли левая кнопка мыши
        return sf::Mouse::isButtonPressed(sf::Mouse::Left);
    }
    return false;
}
