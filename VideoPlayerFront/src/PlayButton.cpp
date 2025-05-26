#include "/Users/andreypavlinich/ProjectPlayer-1/VideoPlayerFront/include/PlayButton.hpp"
#include <iostream>

PlayButton::PlayButton(const sf::Vector2f& position, const sf::Vector2f& size)
    : Button("/Users/andreypavlinich/playButton.png", position, size) {
}

bool PlayButton::onClick(sf::RenderWindow& window) {
    if (this->isMouseOver(window) && sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
        isClicked = true;

        VideoPlayer::Core::MediaPlayer::play();
        return true;
    }
    else {
        isClicked = false;
        return false;
    }
}

void PlayButton::setIsClicked(bool isClicked) {
    this->isClicked = isClicked;
}

bool PlayButton::getIsClicked() {
    return isClicked;
}

void PlayButton::animate(sf::RenderWindow& window) {
    if (this->isMouseOver(window)) {
        this->resize({55, 55}, {312, 297});
     }
     else {
       this->resize({50, 50}, {315, 300});
     }
}
