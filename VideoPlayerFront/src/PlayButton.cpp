#include "/Users/andreypavlinich/ProjectPlayer-1/VideoPlayerFront/include/PlayButton.hpp"
#include <iostream>

PlayButton::PlayButton(const sf::Vector2f& position, const sf::Vector2f& size)
    : Button("/Users/andreypavlinich/playButton.png", position, size) {
    // Дополнительная инициализация, если необходимо
}

void PlayButton::onClick() const {
    std::cout << "Кнопка плей нажата!" << std::endl;
    // Здесь можно добавить логику для кнопки Play
}

void PlayButton::animate(sf::RenderWindow& window) {
    if (this->isMouseOver(window)) {
        this->resize({55, 55}, {312, 297});
     }
     else {
       this->resize({50, 50}, {315, 300});
     }
}
