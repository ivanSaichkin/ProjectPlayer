/* Содержит файлы для рисования кнопки Pause и функции,
создающие эту кнопку */

#ifndef PAUSEBUTTON_HPP
#define PAUSEBUTTON_HPP

#include <SFML/Graphics.hpp>
#include <functional>
#include "Button.hpp"

class PauseButton : public Button {
    public:
        PauseButton(const sf::Vector2f& position, const sf::Vector2f& size);
        void onClick() const;
};

#endif // PAUSEBUTTON_HPP
