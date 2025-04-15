#include "../include/MediaPlayer.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    // Check command line arguments
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <video_file>" << std::endl;
        return 1;
    }

    // Create media player
    MediaPlayer player;

    // Open video file
    if (!player.open(argv[1])) {
        std::cerr << "Failed to open video file: " << argv[1] << std::endl;
        return 1;
    }

    // Run player
    player.run();

    return 0;
}
