#include "../include/MediaPlayer.hpp"
#include <iostream>

void displayHelp(const char* programName) {
    std::cout << "SFML Video Player with FFmpeg\n";
    std::cout << "Usage: " << programName << " <video_file>\n\n";
    std::cout << "Controls:\n";
    std::cout << "  Space: Play/Pause\n";
    std::cout << "  Left Arrow: Seek backward 5 seconds\n";
    std::cout << "  Right Arrow: Seek forward 5 seconds\n";
    std::cout << "  Up Arrow: Increase volume\n";
    std::cout << "  Down Arrow: Decrease volume\n";
    std::cout << "  M: Mute/Unmute\n";
    std::cout << "  F: Toggle fullscreen\n";
    std::cout << "  Escape: Exit fullscreen or quit\n";
}

int main(int argc, char* argv[]) {
    // Check command line arguments
    if (argc < 2) {
        std::cerr << "Error: Missing video file argument\n" << std::endl;
        displayHelp(argv[0]);
        return 1;
    }

    // Handle help flag
    if (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help") {
        displayHelp(argv[0]);
        return 0;
    }

    try {
        // Create media player
        MediaPlayer player;

        // Set custom error callback
        player.setErrorCallback([](const MediaPlayerException& error) {
            std::cerr << "Error: " << error.what() << std::endl;

            // Exit on critical errors
            if (error.getCode() == MediaPlayerException::FILE_NOT_FOUND) {
                std::exit(1);
            }
        });

        // Open video file
        if (!player.open(argv[1])) {
            return 1;
        }

        // Run player
        player.run();

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
