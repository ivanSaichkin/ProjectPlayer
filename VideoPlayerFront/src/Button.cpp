#include "/Users/andreypavlinich/ProjectPlayer-1/VideoPlayerFront/include/Button.hpp"
#include <iostream>

Button::Button() { }

Button::Button(const std::string& texturePath, const sf::Vector2f& position, const sf::Vector2f& size) {
    // Загружаем текстуру
    if (!texture.loadFromFile(texturePath)) {
        std::cerr << "Failed to load button texture: " << texturePath << std::endl;
    }

    // Устанавливаем текстуру для спрайта
    sprite.setTexture(texture);

    // Устанавливаем размер и позицию
    setSize(size);
    setPosition(position);
}

// Реализация оператора присваивания
Button& Button::operator=(const Button& other) {
    if (this == &other) { // Проверка на самоприсваивание
        return *this;
    }

    // Копируем текстуру
    texture = other.texture;

    // Копируем спрайт
    sprite = other.sprite;
    sprite.setTexture(texture); // Устанавливаем текстуру для спрайта

    // Копируем callback
    onClickCallback = other.onClickCallback;

    return *this;
}

void Button::draw(sf::RenderWindow& window) const {
    window.draw(sprite); // Рисуем спрайт
}

bool Button::isMouseOver(const sf::RenderWindow& window) const {
    sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    return sprite.getGlobalBounds().contains(mousePos); // Проверяем, находится ли курсор над спрайтом
}

void Button::onClick(std::function<void()> callback) {
    onClickCallback = callback;
}

void Button::setPosition(const sf::Vector2f& position) {
    sprite.setPosition(position); // Устанавливаем позицию спрайта
}

void Button::setSize(const sf::Vector2f& size) {
    // Масштабируем спрайт до нужного размера
    sf::FloatRect bounds = sprite.getLocalBounds();
    sprite.setScale(size.x / bounds.width, size.y / bounds.height);
}

void Button::updateSprite() {
    // Этот метод можно использовать для дополнительных обновлений спрайта, если необходимо
}

// Реализация геттеров

// Получить текстуру
const sf::Texture& Button::getTexture() const {
    return texture;
}

// Получить спрайт
const sf::Sprite& Button::getSprite() const {
    return sprite;
}

// Получить callback
std::function<void()> Button::getOnClickCallback() const {
    return onClickCallback;
}

void Button::setTexture(const sf::Texture& newTexture) {
    texture = newTexture;
    sprite.setTexture(texture); // Обновляем текстуру спрайта
}

void Button::setSprite(const sf::Sprite& newSprite) {
    sprite = newSprite;
    sprite.setTexture(texture); // Убедимся, что текстура спрайта обновлена
}

void Button::setOnClickCallback(std::function<void()> newCallback) {
    onClickCallback = newCallback;
}


// Используется когда наводим мышку на кнопку
void Button::resize(const sf::Vector2f& size, const sf::Vector2f& position) {
    this->setSize(size);
    this->changePosition(position);
}

void Button::changePosition(const sf::Vector2f& position) {
    sprite.setPosition(position);
}
