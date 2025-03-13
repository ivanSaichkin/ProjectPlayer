#include "/Users/andreypavlinich/a/PlayerRep/ProjectPlayer/VideoPlayer/include/PlayButton.hpp"
#include <iostream>

namespace PlayButton {
    bool LoadTexture(sf::Texture& texture) {
        if (!texture.loadFromFile("/Users/andreypavlinich/playButton.png")) {
            std::cerr << "Текстура кнопки Play не найдена!" << std::endl;
            return false;
        }
        return true;
    }

    sf::Sprite CreateSprite(sf::Texture& texture) {
        sf::Sprite sprite;
        sprite.setTexture(texture);
        return sprite;
    }

    void CreatePlayButton(Button& playButton) {
        playButton.setLabel("");
        playButton.setPosition({50, 300});
        playButton.setSize({50, 50});
    }
}
