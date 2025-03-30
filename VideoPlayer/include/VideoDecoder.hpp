#ifndef VIDEODECODER_HPP
#define VIDEODECODER_HPP

#include <queue>

#include "Decoder.hpp"
#include "MediaFile.hpp"
#include "SFML/Graphics.hpp"

class VideoDecoder : public Decoder {
 public:
    VideoDecoder(const MediaFile& mediaFile);
    ~VideoDecoder();
    void Draw(sf::RenderWindow& window);

    void Start() override;
    void Flush() override;
    void ProcessPacket(AVPacket* packet) override;
    void SignalEndOfStream() override;

    sf::Sprite& GetSprite();
    sf::Vector2i GetSize() const;

 private:
    void DecodeVideo();
    void ProcessVideoFrame(AVFrame* frame);

    AVCodecContext* videoCodecContext_;
    AVFrame* videoFrame_;
    SwsContext* swsContext_;
    sf::Texture texture_;
    sf::Sprite sprite_;
    MediaFile mediaFile_;
};

class VideoDecoderError : public std::runtime_error {
 public:
    explicit VideoDecoderError(const std::string& errMessage) : std::runtime_error(errMessage) {}
};

#endif
