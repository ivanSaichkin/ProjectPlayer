#ifndef BUTTON_HPP
#define BUTTON_HPP

#include <SFML/Graphics.hpp>
#include <functional>
#include <string>
#include "/Users/andreypavlinich/ProjectPlayer-1/VideoPlayerBack/include/core/MediaPlayerManager.hpp"

class Button {
public:
    Button(const std::string& texturePath, const sf::Vector2f& position, const sf::Vector2f& size);
    Button();

    // Оператор присваивания
    Button& operator=(const Button& other);

    virtual void draw(sf::RenderWindow& window) const; //
    bool isMouseOver(const sf::RenderWindow& window) const;
    void onClick(std::function<void()> callback);

    void setPosition(const sf::Vector2f& position);
    void setSize(const sf::Vector2f& size);

    // Геттеры
    const sf::Texture& getTexture() const; // Получить текстуру
    const sf::Sprite& getSprite() const;  // Получить спрайт
    std::function<void()> getOnClickCallback() const; // Получить callback

    // Сеттеры
    void setTexture(const sf::Texture& newTexture); // Установить текстуру
    void setSprite(const sf::Sprite& newSprite);   // Установить спрайт
    void setOnClickCallback(std::function<void()> newCallback); // Установить callback

    // Вспомогательные методы (для интерактивной работы)
    void resize(const sf::Vector2f& size, const sf::Vector2f& position);
    void changePosition(const sf::Vector2f& position);

protected:
    sf::Texture texture; // Текстура для кнопки
    sf::Sprite sprite;   // Спрайт для отображения текстуры
    std::function<void()> onClickCallback;

    void updateSprite(); // Обновляет спрайт после изменения позиции или размера
};

#endif // BUTTON_HPP
