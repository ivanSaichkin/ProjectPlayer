/* Тест программы */
#include "/Users/andreypavlinich/a/PlayerRep/ProjectPlayer/VideoPlayer/include/Button.hpp"
#include "/Users/andreypavlinich/a/PlayerRep/ProjectPlayer/VideoPlayer/include/PlayButton.hpp"
#include "/Users/andreypavlinich/a/PlayerRep/ProjectPlayer/VideoPlayer/include/PauseButton.hpp"
#include "/Users/andreypavlinich/a/PlayerRep/ProjectPlayer/VideoPlayer/include/ProgressBar.hpp"
#include <SFML/Graphics.hpp>
#include <iostream>

int main() {
  sf::Font font;
  if (!font.loadFromFile("/System/Library/Fonts/Supplemental/Arial.ttf")) {
    return -1;
  }

  PlayButton playButton({315, 300}, {50, 50});   // создаём кнопку Play
  PauseButton pauseButton({245, 300}, {50, 50});
  ProgressBar progressBar({50, 360}, {500, 10}, sf::Color::White, sf::Color::Green);
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
    progressBar.draw(window);
    window.display();

    sf::Sprite playButtonSprite = playButton.getSprite();

    // анимирование кнопок
    if (playButton.isMouseOver(window)) {
       playButton.resize({55, 55}, {312, 297});
    }
    else {
      playButton.resize({50, 50}, {315, 300});
    }

    if (pauseButton.isMouseOver(window)) {
      pauseButton.resize({55, 55}, {242, 297});
    }
    else {
      pauseButton.resize({50, 50}, {245, 300});
    }


    /*
    здесь (далее) реализовать обновление положения ползунка в зависимости
    от положения курсора мышки (и возможность перемотки нажатием в т.ч.)
    */

    if (progressBar.isThumbClicked(window)) {
      progressBar.setThumbColor(sf::Color(146, 208, 231)); // мб цвета в структуру/класс поместить, чтобы не путаться
      // ...
    }
    else {
      progressBar.setThumbColor(sf::Color(26, 181, 239));
    }

  }
  // конец основного цикла
  return 0;
}
