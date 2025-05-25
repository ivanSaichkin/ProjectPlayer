#include "/Users/andreypavlinich/ProjectPlayer-1/VideoPlayerFront/include/PauseButton.hpp"
#include <iostream>

PauseButton::PauseButton(const sf::Vector2f& position, const sf::Vector2f& size)
    : Button("/Users/andreypavlinich/pauseButton.png", position, size) {
    // Дополнительная инициализация, если необходимо
}

void PauseButton::onClick() const {
    std::cout << "Кнопка паузы нажата!" << std::endl;
    // Здесь можно добавить логику для кнопки Pause
}

void PauseButton::animate(sf::RenderWindow& window){
    if (this->isMouseOver(window)) {
       this->resize({55, 55}, {242, 297});
      }
      else {
        this->resize({50, 50}, {245, 300});
      }
}
