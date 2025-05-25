#include "/Users/andreypavlinich/a/PlayerRep/ProjectPlayer/VideoPlayer/include/PlayButton.hpp"
#include <iostream>

PlayButton::PlayButton(const sf::Vector2f& position, const sf::Vector2f& size)
    : Button("/Users/andreypavlinich/playButton.png", position, size) {
    // Дополнительная инициализация, если необходимо
}

void PlayButton::onClick() const {
    std::cout << "Кнопка плей нажата!" << std::endl;
    // Здесь можно добавить логику для кнопки Play
}
