#pragma once

#include "/Users/andreypavlinich/a/PlayerRep/ProjectPlayer/VideoPlayer/include/Button.hpp"
#include "/Users/andreypavlinich/ProjectPlayer-1/VideoPlayerFront/include/PauseButton.hpp"
#include "/Users/andreypavlinich/ProjectPlayer-1/VideoPlayerFront/include/PlayButton.hpp"
#include "/Users/andreypavlinich/ProjectPlayer-1/VideoPlayerFront/include/ProgressBar.hpp"
#include "/Users/andreypavlinich/ProjectPlayer-1/VideoPlayerFront/include/window/window.hpp"
#include <SFML/Graphics.hpp>
#include <iostream>
#include <thread>
#include <chrono>

int main() {

  PlayButton playButton({315, 300}, {50, 50});   // создаём кнопку Play
  PauseButton pauseButton({245, 300}, {50, 50});
  ProgressBar progressBar({50, 360}, {500, 10}, sf::Color::White, sf::Color::Green);

  ButtonWindow::openButtonWindow(playButton, pauseButton, progressBar);
  return 0;
}
