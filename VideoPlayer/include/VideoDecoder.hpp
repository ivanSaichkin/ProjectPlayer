#ifndef VIDEODECODER_HPP
#define VIDEODECODER_HPP

#include "Decoder.hpp"
#include "MediaFile.hpp"
#include "SFML/Graphics.hpp"

class VideoDecoder : public Decoder {
 public:
    VideoDecoder(const MediaFile& mediaFile);
    ~VideoDecoder();
    void Start() override;
    void Stop() override;
    void Draw(sf::RenderWindow& window);
    void TogglePause() override;
    void Flush() override;
    void SetStartTime(std::chrono::steady_clock::time_point startTime) override;

 private:
    void DecodeVideo();

    AVCodecContext* videoCodecContext_;
    AVFrame* videoFrame_;
    SwsContext* swsContext_;
    sf::Texture texture_;
    sf::Sprite sprite_;
    MediaFile mediaFile_;
};

#endif
