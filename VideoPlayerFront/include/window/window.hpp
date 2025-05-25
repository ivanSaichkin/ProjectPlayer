#pragma once

#include "/Users/andreypavlinich/a/PlayerRep/ProjectPlayer/VideoPlayer/include/Button.hpp"
#include "/Users/andreypavlinich/ProjectPlayer-1/VideoPlayerFront/include/PauseButton.hpp"
#include "/Users/andreypavlinich/ProjectPlayer-1/VideoPlayerFront/include/PlayButton.hpp"
#include "/Users/andreypavlinich/ProjectPlayer-1/VideoPlayerFront/include/ProgressBar.hpp"
#include "/Users/andreypavlinich/ProjectPlayer-1/VideoPlayerFront/include/window/window.hpp"
#include <SFML/Graphics.hpp>
#include <iostream>
#include <thread>

namespace Colors {
    inline sf::Color lightBlue{146, 208, 231};
    inline sf::Color windowColor{79, 95, 102};
}

namespace Window {
    const int windowSizeY = 400;
    const int windowSizeX = 600;
}

namespace ButtonWindow {
    // sf::RenderWindow createWindow(int sizeX, int sizeY);
    void startWindowCycle(PlayButton& playButton, PauseButton& pauseButton, ProgressBar& progressBar);

    void drawButtons(sf::RenderWindow& window, PlayButton& playButton, PauseButton& PauseButton, ProgressBar& progressBar);

    void progressBarProcessing(sf::RenderWindow& window, ProgressBar& progressBar);
    void playButtonProcessing(sf::RenderWindow& window, PlayButton& playButton);
    void pauseButtonProcessing(sf::RenderWindow& window, PauseButton& pauseButton);

    void process();
}
