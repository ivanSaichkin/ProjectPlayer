#include "/Users/andreypavlinich/a/PlayerRep/ProjectPlayer/VideoPlayer/include/PlayButton.hpp"
#include <iostream>

namespace PlayButton {
bool LoadTexture(sf::Texture &texture) {
  if (!texture.loadFromFile("/Users/andreypavlinich/playButton.png")) {
    std::cerr << "Текстура кнопки Play не найдена!" << std::endl;
    return false;
  }
  return true;
}

sf::Sprite CreateSprite(sf::Texture &texture) {
  sf::Sprite sprite;
  sprite.setTexture(texture);
  return sprite;
}

Button CreatePlayButton() {
  Button playButton("/Users/andreypavlinich/playButton.png", {245, 300},
                    {50, 50});

  sf::Texture playButtonTexture;
  LoadTexture(playButtonTexture);
  playButtonTexture.setSmooth(true);

  sf::Sprite playButtonSprite = CreateSprite(playButtonTexture);

  return playButton;
}
} // namespace PlayButton
