/* Содержит файлы для рисования кнопки Play и функции,
создающие эту кнопку */

#ifndef PLAYBUTTON_HPP
#define PLAYBUTTON_HPP

#include <SFML/Graphics.hpp>
#include <functional>
#include "Button.hpp"

namespace PlayButton {
bool LoadTexture(sf::Texture& texture);
Button CreatePlayButton();
sf::Sprite CreateSprite(sf::Texture& texture);
}



#endif // PLAYBUTTON_HPP
