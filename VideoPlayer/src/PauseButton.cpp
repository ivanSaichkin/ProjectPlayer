#include "/Users/andreypavlinich/a/PlayerRep/ProjectPlayer/VideoPlayer/include/PauseButton.hpp"
#include <iostream>

namespace PauseButton {
bool LoadTexture(sf::Texture &texture) {
  if (!texture.loadFromFile(
          "/Users/andreypavlinich/pauseButton.png")) { // Сюда путь к текстуре
                                                       // кнопки
    std::cerr << "Текстура кнопки Pause не найдена!" << std::endl;
    return false;
  }
  return true;
}

sf::Sprite CreateSprite(sf::Texture &texture) {
  sf::Sprite sprite;
  sprite.setTexture(texture);
  return sprite;
}

Button CreatePauseButton() {
  Button pauseButton("/Users/andreypavlinich/pauseButton.png", {315, 300},
                     {50, 50});

  sf::Texture pauseButtonTexture;
  LoadTexture(pauseButtonTexture);
  pauseButtonTexture.setSmooth(true);

  sf::Sprite pauseButtonSprit = CreateSprite(pauseButtonTexture);

  return pauseButton;
}
} // namespace PauseButton
