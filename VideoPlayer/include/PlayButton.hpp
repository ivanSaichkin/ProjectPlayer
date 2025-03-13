/* Содержит файлы для рисования кнопки play и функции,
создающие эту кнопку */


#ifndef PLAYBUTTON_HPP
#define PLAYBUTTON_HPP

#include <SFML/Graphics.hpp>
#include <functional>
#include "Button.hpp"

namespace PlayButton {
bool LoadTexture(sf::Texture& texture);
void CreatePlayButton(Button& playButton);
sf::Sprite CreateSprite(sf::Texture& texture);
}



#endif // PLAYBUTTON_HPP
