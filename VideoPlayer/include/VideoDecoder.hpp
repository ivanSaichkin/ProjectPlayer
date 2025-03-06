#ifndef VIDEODECODER_HPP
#define VIDEODECODER_HPP

#include "Decoder.hpp"
#include "MediaFile.hpp"
#include "SFML/Graphics.hpp"

class VideoDecoder : public Decoder {
 public:
    VideoDecoder(const MediaFile& mediaFile);
    void Start() override;
    void Stop() override;
    void Draw(sf::RenderWindow& window);

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
