#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "VideoDecoder.hpp"
#include "AudioDecoder.hpp"
#include "MediaFile.hpp"

class Player {
 public:
    Player();
    void Load(const std::string& filename);
    void Play();
    void TogglePause();
    void Stop();
    void Draw(sf::RenderWindow& window);
    void Seek(int seconds);
    void SetVolume(float volume);

 private:
    MediaFile mediaFile_;
    std::unique_ptr<VideoDecoder> videoDecoder_;
    std::unique_ptr<AudioDecoder> audioDecoder_;
};

#endif
