#include "/Users/andreypavlinich/a/PlayerRep/ProjectPlayer/VideoPlayer/include/Button.hpp"
#include "/Users/andreypavlinich/a/PlayerRep/ProjectPlayer/VideoPlayer/include/PlayButton.hpp"
#include <SFML/Graphics.hpp>
#include <iostream>

int main() {
  sf::Font font;
  if (!font.loadFromFile("/System/Library/Fonts/Supplemental/Arial.ttf")) {
    std::cerr << "Failed to load font!" << std::endl;
    return -1;
  }

  // создаём кнопку Play
  Button playButton;
  PlayButton::CreatePlayButton(playButton);

  // создаем текстуру кнопки
  sf::Texture playButtonTexture;
  // Если текстура не загружена, завершаем программу
  if (!PlayButton::LoadTexture(playButtonTexture)) {
    return -1;
  }

  // создаем спрайт и устанавливаем текстуру
  sf::Sprite playButtonSprite = PlayButton::CreateSprite(playButtonTexture);

  sf::Vector2u textureSize =
      playButtonTexture.getSize(); // Получаем размер текстуры
  float desiredWidth = 200.0f;     // Желаемая ширина
  float desiredHeight = 200.0f; // Желаемая высота
  float scaleX =
      desiredWidth / static_cast<float>(textureSize.x); // Масштаб по X
  float scaleY =
      desiredHeight / static_cast<float>(textureSize.y); // Масштаб по Y
  playButtonSprite.setScale(scaleX, scaleY); // Применяем масштаб
                                             // рендерим окно
  sf::RenderWindow window(sf::VideoMode(600, 400), L"Новый проект",
                          sf::Style::Default);
  // включаем вертикальную синхронизацию (пока что так)
  window.setVerticalSyncEnabled(true);
  window.clear(sf::Color::White);
  // старт основного цикла
  // Центрируем спрайт в окне
  playButtonSprite.setPosition((window.getSize().x - desiredWidth) / 2.0f,
                               (window.getSize().y - desiredHeight) / 2.0f);
  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      if (event.type == sf::Event::Closed) {
        window.close();
      }
    }

    window.clear();
    window.draw(playButtonSprite); // Отрисовываем спрайт
    window.display();
  }
  // конец основного цикла
  return 0;
}
