/* Тест программы */

#include "/Users/andreypavlinich/a/PlayerRep/ProjectPlayer/VideoPlayer/include/Button.hpp"
#include "/Users/andreypavlinich/a/PlayerRep/ProjectPlayer/VideoPlayer/include/PlayButton.hpp"
#include "/Users/andreypavlinich/a/PlayerRep/ProjectPlayer/VideoPlayer/include/PauseButton.hpp"
#include <SFML/Graphics.hpp>
#include <iostream>

int main() {
  sf::Font font;
  if (!font.loadFromFile("/System/Library/Fonts/Supplemental/Arial.ttf")) {
    return -1;
  }

  PlayButton playButton({335, 300}, {50, 50});   // создаём кнопку Play
  PauseButton pauseButton({265, 300}, {50, 50});
  sf::RenderWindow window(sf::VideoMode(600, 400), L"Новый проект",
                          sf::Style::Default);
  window.setVerticalSyncEnabled(true);   // включаем вертикальную синхронизацию (пока что так)
  window.clear(sf::Color::Black);

  // старт основного цикла
  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      if (event.type == sf::Event::Closed) {
        window.close();
      }
    }

    // sf::RenderStates states; хз надо ли
    // states.blendMode = sf::BlendAlpha; хз надо ли

    // Отрисовываем knopki
    window.clear();
    playButton.draw(window);
    pauseButton.draw(window);
    window.display();
  }
  // конец основного цикла
  return 0;
}
