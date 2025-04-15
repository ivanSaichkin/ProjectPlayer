#include <SFML/Graphics.hpp>
#include <iostream>

#include "../API/MediaPlayer.hpp"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <video_file>" << std::endl;
        return 1;
    }

    // Create media player
    MediaPlayer player;

    // Set up error handling
    player.setErrorCallback([](const MediaPlayerException& e) { std::cerr << "Error: " << e.what() << std::endl; });

    // Open video file
    if (!player.open(argv[1])) {
        std::cerr << "Failed to open video file" << std::endl;
        return 1;
    }

    // Print media information
    std::cout << "Duration: " << player.getDuration() << " seconds" << std::endl;
    sf::Vector2u size = player.getVideoSize();
    std::cout << "Video size: " << size.x << "x" << size.y << std::endl;
    std::cout << "Frame rate: " << player.getFrameRate() << " fps" << std::endl;

    // Create window
    sf::RenderWindow window(sf::VideoMode(size.x, size.y), "Video Player");
    sf::Sprite sprite;

    // Start playback
    player.play();

    // Main loop
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            } else if (event.type == sf::Event::KeyPressed) {
                switch (event.key.code) {
                    case sf::Keyboard::Space:
                        player.togglePlayPause();
                        break;
                    case sf::Keyboard::Left:
                        player.seek(player.getCurrentPosition() - 5.0);
                        break;
                    case sf::Keyboard::Right:
                        player.seek(player.getCurrentPosition() + 5.0);
                        break;
                    case sf::Keyboard::Escape:
                        window.close();
                        break;
                    default:
                        break;
                }
            }
        }

        // Update player
        player.update();

        // Get current frame
        sf::Texture texture;
        if (player.getCurrentFrame(texture)) {
            sprite.setTexture(texture, true);
        }

        // Render
        window.clear();
        window.draw(sprite);
        window.display();
    }

    // Clean up
    player.close();

    return 0;
}
