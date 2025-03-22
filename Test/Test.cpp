#include <iostream>

#include "../VideoPlayer/include/Player.hpp"
#include "SFML/Graphics.hpp"

int main(int argc, char* argv[]) {
    // Initialize FFmpeg
    av_log_set_level(AV_LOG_QUIET);

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <video_file>" << std::endl;
        return 1;
    }

    std::string filename = argv[1];

    // Create window
    sf::RenderWindow window(sf::VideoMode(800, 600), "Video Player");
    window.setFramerateLimit(60);

    // Create player
    Player player;

    try {
        // Load video
        player.Load(filename);

        // Set initial volume (0-100)
        player.SetVolume(50.0f);

        // Start playback
        player.Play();

        // Main loop
        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    window.close();
                } else if (event.type == sf::Event::KeyPressed) {
                    switch (event.key.code) {
                        case sf::Keyboard::Space:
                            player.TogglePause();
                            break;
                        case sf::Keyboard::Escape:
                            window.close();
                            break;
                        case sf::Keyboard::Left:
                            player.Seek(-10);  // Seek 10 seconds backward
                            break;
                        case sf::Keyboard::Right:
                            player.Seek(10);  // Seek 10 seconds forward
                            break;
                        default:
                            break;
                    }
                }
            }

            // Clear window
            window.clear();

            // Draw video frame
            player.Draw(window);

            // Display window
            window.display();
        }

        // Stop playback
        player.Stop();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
