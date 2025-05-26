#include "/Users/andreypavlinich/ProjectPlayer-1/VideoPlayerFront/include/PauseButton.hpp"
#include <iostream>

PauseButton::PauseButton(const sf::Vector2f& position, const sf::Vector2f& size)
    : Button("/Users/andreypavlinich/pauseButton.png", position, size) {
}

bool PauseButton::onClick(sf::RenderWindow& window) { // проверка ЛКМ
    if (this->isMouseOver(window) && sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
        isClicked = true;

        VideoPlayer::Core::MediaPlayer::pause();
        return true;
    }
    else {
        isClicked = false;
        return false;
    }
}

void PauseButton::setIsClicked(bool isClicked) {
    this->isClicked = isClicked;
}

bool PauseButton::getIsClicked() {
    return isClicked;
}

void PauseButton::animate(sf::RenderWindow& window){
    if (this->isMouseOver(window)) {
       this->resize({55, 55}, {242, 297});
      }
      else {
        this->resize({50, 50}, {245, 300});
      }
}
