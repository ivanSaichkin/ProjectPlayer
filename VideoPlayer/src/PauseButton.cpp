#include "/Users/andreypavlinich/a/PlayerRep/ProjectPlayer/VideoPlayer/include/PauseButton.hpp"
#include <iostream>

PauseButton::PauseButton(const sf::Vector2f& position, const sf::Vector2f& size)
    : Button("/Users/andreypavlinich/pauseButton.png", position, size) {
    // Дополнительная инициализация, если необходимо
}

void PauseButton::onClick() const {
    std::cout << "Pause button clicked!" << std::endl;
    // Здесь можно добавить логику для кнопки Pause
}
