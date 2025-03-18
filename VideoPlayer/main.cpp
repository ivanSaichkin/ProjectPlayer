/* Тест программы */

#include "/Users/andreypavlinich/a/PlayerRep/ProjectPlayer/VideoPlayer/include/Button.hpp"
#include "/Users/andreypavlinich/a/PlayerRep/ProjectPlayer/VideoPlayer/include/PlayButton.hpp"
#include "/Users/andreypavlinich/a/PlayerRep/ProjectPlayer/VideoPlayer/include/PauseButton.hpp"
#include "/Users/andreypavlinich/a/PlayerRep/ProjectPlayer/VideoPlayer/include/ScrollBar.hpp"
#include <SFML/Graphics.hpp>
#include <iostream>

int main() {
  sf::Font font;
  if (!font.loadFromFile("/System/Library/Fonts/Supplemental/Arial.ttf")) {
    return -1;
  }

  PlayButton playButton({315, 300}, {50, 50});   // создаём кнопку Play
  PauseButton pauseButton({245, 300}, {50, 50});
  ScrollBar scrollBar({50, 360}, {500, 10}, sf::Color::White, sf::Color::Green);
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
    scrollBar.draw(window);
    window.display();
  }
  // конец основного цикла
  return 0;
}
