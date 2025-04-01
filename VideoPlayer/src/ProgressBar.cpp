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
    updateThumbPosition(position);

    // Устанавливаем размер и позицию для базового класса Button
    setPosition(position);
    setSize(size);
}

void ProgressBar::draw(sf::RenderWindow& window) const {
    window.draw(track); // Рисуем фон
    window.draw(thumb); // Рисуем бегунок
}

void ProgressBar::onClick(std::function<void(float)> callback) {
    onClickCallback = callback; // Сохраняем callback (а надо ли его ваще использовать??)
}

bool ProgressBar::isMouseOverThumb(const sf::RenderWindow& window) const {
    sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    // Проверяем, находится ли курсор над бегунком (кругом)
    float distanceSquared = std::pow(mousePos.x - thumb.getPosition().x, 2) + std::pow(mousePos.y - thumb.getPosition().y, 2);
    return distanceSquared <= std::pow(thumb.getRadius(), 2);
}

bool ProgressBar::isMouseOverTrack(const sf::RenderWindow& window) const {
    sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    sf::FloatRect bounds = track.getGlobalBounds();
    return bounds.contains(mousePos);
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

const sf::CircleShape& ProgressBar::getThumb() const {
    return thumb;
}

const sf::RectangleShape& ProgressBar::getTrack() const {
    return track;
}

void ProgressBar::updateThumbPosition(const sf::Vector2f& position) {
    float thumbX = position.x;
    float thumbY = track.getPosition().y + 5; // перемещение ползунка по Оy (можно поменять на track.getPosition().y)
    thumb.setPosition(thumbX, thumbY);
}

bool ProgressBar::isThumbClicked(const sf::RenderWindow& window) const {
    // Проверяем, находится ли курсор мыши над бегунком
    if (isMouseOver(window)) {
        // Проверяем, нажата ли левая кнопка мыши
        return sf::Mouse::isButtonPressed(sf::Mouse::Left);
    }
    return false;
}


bool ProgressBar::isTrackClicked(const sf::RenderWindow& window) const {
    if (isMouseOverTrack(window)) {
    return sf::Mouse::isButtonPressed(sf::Mouse::Left);
    }
    return false;
}

void ProgressBar::updateThumbFromMouse(const sf::RenderWindow& window) {
        if (!isDragging) return;

        // Получаем текущую позицию мыши
        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

        // Рассчитываем границы для ползунка
        float minX = track.getPosition().x + thumb.getRadius();
        float maxX = track.getPosition().x + track.getSize().x - thumb.getRadius();

        // Ограничиваем позицию мыши границами трека
        float thumbX = std::clamp(mousePos.x, minX, maxX);

        // Устанавливаем новую позицию ползунка
        thumb.setPosition(thumbX, track.getPosition().y + track.getSize().y / 2.f);

        // Обновляем прогресс (0..1) на основе новой позиции
        progress = (thumbX - minX) / (maxX - minX);
 }

void ProgressBar::startDragging() {
     isDragging = true;
    }

void ProgressBar::stopDragging() {
    isDragging = false;
 }

bool ProgressBar::getIsDragging() const {
    return isDragging;
 }


 /*
  План по прогресс бару:
 1. Разделить его длину на несколько частей
 2. Получить из "ВидеоДекодера" длительность конкретного видео
 3. Разделить длительность видео на то же количество частей, что и длину прогрессбара
 4. Каждый раз по прохождении видеоматриалом n-й части длительности самого видео,
  сдвигать ползунок на соответствующее количество частей
 5. Связать положение ползунка длительностью видео (прогрессом)
*/
