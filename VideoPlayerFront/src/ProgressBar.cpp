#include "/Users/andreypavlinich/ProjectPlayer-1/VideoPlayerFront/include/Button.hpp"
#include "/Users/andreypavlinich/ProjectPlayer-1/VideoPlayerFront/include/ProgressBar.hpp"
#include <iostream>
#include <thread>
#include <chrono>


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
    setDuration(500);
    setProgress(0);
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

void ProgressBar::setDuration(const int& duration) {
    this->duration = duration;
}

void ProgressBar::setProgress(const int& progress) {
    this->progress = progress;
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
    duration = abs(position.x - track.getPosition().x) / track.getLocalBounds().width * 10000;
}

void ProgressBar::fillWithColor(const sf::Vector2f& position) {
    float rightBorder = position.x;
    sf::RectangleShape trackCover;
    sf::Vector2f size = track.getSize();
    sf::Vector2f sizeOfCover = {rightBorder, size.y};

    trackCover.setFillColor(sf::Color::Green);


}

bool ProgressBar::isThumbClicked(const sf::RenderWindow& window) const {
    // Проверяем, находится ли курсор мыши над бегунком
    if (isMouseOver(window)) {
        // Проверяем, нажата ли левая кнопка мыши
        return sf::Mouse::isButtonPressed(sf::Mouse::Left);
        if (isMouseOverThumb(window)) {
            return true;
        }
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

 int ProgressBar::getDuration() {
    return duration;
 }

/*

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
Сначала получаем длительность видео (duration),
потом преобразуем ее в progress, т.е. прогресс меняем в зависимости от duration
а потом обновляем данные на прогресс баре.
durationFull можно сделать константным полем, получаемым в самом начале (при запуске видео)
это нужно для перерасчёта duration при изменении положения ползунка
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

прогресс разделен на 100 частей

что происходит:
1. Обновляется положение ползунка
2. Настоящая Длительность (duration) является полем, и тоже меняется при изменении положения ползунка
3. Сначала меняем прогресс, потом от него длительность (duration / 100 * progress)

progress принадлежит от 1 до 100
*/


/*
План (новый) :
1. Сделать единую функцию отслеживания/изменени
положения ползунка в зависимости от :нажатия кнопок, мыши,
момента времени видео (возможно прогресс не нужен, целочисленное
деление работает в UpdateThumbFromProgress(...) )
2. Можно отказаться от поля progress (оно не понадобилось),
или заменить название поля duration на него
3. С каждым внешним изменением положения ползунка (внешнее,
т.е. его делает пользователь), нужно менять поле duration, а
с каждым изменением duration менять положение ползунка - это
в функции UpdateThumbFromProgress(...))

*/

void ProgressBar::UpdateThumbFromProgress(const sf::RenderWindow& window) {
    if (!isTrackClicked(window)) {

    sf::FloatRect progressBarSize = track.getLocalBounds();
    float progressBarWidth = progressBarSize.width;

    int durationFull = 10000; //длительность видео

    float elementaryProgress = progressBarWidth / 100;

    progress = (duration / durationFull * 100); // МОЖНО УБРАТЬ!

    ProgressBar::updateThumbPosition({(progressBarWidth * duration / durationFull) + track.getPosition().x, 100}); // ВАЖНЫЙ РАСЧЕТ ПОЛОЖЕНИЯ ПОЛЗУНКА ЧЕРЕЗ ВРЕМЯ

    duration += 10;
    }

    else {
        return;
    }
}
