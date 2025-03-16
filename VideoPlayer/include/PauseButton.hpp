/* Содержит файлы для рисования кнопки Pause и функции,
создающие эту кнопку */

#ifndef PAUSEBUTTON_HPP
#define PAUSEBUTTON_HPP

#include <SFML/Graphics.hpp>
#include <functional>
#include "Button.hpp"

namespace PauseButton {
bool LoadTexture(sf::Texture& texture);
Button CreatePauseButton();
sf::Sprite CreateSprite(sf::Texture& texture);
}



#endif // PAUSEBUTTON_HPP
