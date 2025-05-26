/* Содержит файлы для рисования кнопки Play и функции,
создающие эту кнопку */

#ifndef PLAYBUTTON_HPP
#define PLAYBUTTON_HPP

#include <SFML/Graphics.hpp>
#include <functional>
#include "Button.hpp"

class PlayButton : public Button {
public:
    PlayButton(const sf::Vector2f& position, const sf::Vector2f& size);
    bool onClick(sf::RenderWindow& window); // Переопределяем метод для специфичного поведения
    void animate(sf::RenderWindow& window);

    void getIsClicked();
    void setIsClicked(bool isClicked);
private:
    bool isClicked;
};

#endif // PLAYBUTTON_HPP
