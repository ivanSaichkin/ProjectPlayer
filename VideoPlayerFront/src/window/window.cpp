#include "/Users/andreypavlinich/ProjectPlayer-1/VideoPlayerFront/include/window/window.hpp"
#include <iostream>

namespace ButtonWindow
{
    // sf::RenderWindow createWindow(int sizeX, int sizeY) {
    //     sf::RenderWindow window(sf::VideoMode(600, 400), L"Buttons", sf::Style::Default);

    //     window.setVerticalSyncEnabled(true);

    //     return window;
    // }

    void openButtonWindow(PlayButton& playButton, PauseButton& pauseButton, ProgressBar& progressBar) {
        sf::RenderWindow window(sf::VideoMode(600, 400), L"Buttons", sf::Style::Default);

        window.setVerticalSyncEnabled(true);

        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
              if (event.type == sf::Event::Closed) {
                window.close();
              }
              window.clear(sf::Color(79, 95, 102));

              drawButtons(window, playButton, pauseButton, progressBar);

              playButtonProcessing(window, playButton);
              pauseButtonProcessing(window, pauseButton);
              progressBarProcessing(window, progressBar);
            }
    }
}

    void drawButtons(sf::RenderWindow& window, PlayButton& playButton, PauseButton& pauseButton, ProgressBar& progressBar) {
        playButton.draw(window);
        pauseButton.draw(window);
        progressBar.draw(window);

        window.display();
    }

    void progressBarProcessing(sf::RenderWindow& window, ProgressBar& progressBar) {
        float progress = 0.f;
        if (progressBar.isThumbClicked(window)) {
          progressBar.setThumbColor(sf::Color(146, 208, 231));
          sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        }

        else {
          progressBar.setThumbColor(sf::Color(26, 181, 239));
        }

        // функция UpdateThumbFromMouse(window)
        if (progressBar.isTrackClicked(window)) {
          sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
            progressBar.updateThumbPosition(mousePos);
         }

        // Функция void UpdateThumbFromProgress(window)

         progressBar.UpdateThumbFromProgress(window);
        //  std::cout << progressBar.getDuration();
        // Функция ShowDuration(window)
      }

    void playButtonProcessing(sf::RenderWindow& window, PlayButton& playButton) {
        playButton.animate(window);
    }

    void pauseButtonProcessing(sf::RenderWindow& window, PauseButton& pauseButton) {
        pauseButton.animate(window);
    }



} // namespace ButtonWindow
