#ifndef AUDIODECODER_HPP
#define AUDIODECODER_HPP

#include "Decoder.hpp"
#include "MediaFile.hpp"
#include "SFML/Audio.hpp"

class AudioDecoder : public Decoder {
 public:
    AudioDecoder(const MediaFile& mediaFile);
    ~AudioDecoder();
    void SetVolume(float volume);

    void Start() override;
    void Flush() override;

 private:
    void DecodeAudio();

    AVCodecContext* audioCodecContext_;
    sf::Sound sound_;
    sf::SoundBuffer soundBuffer_;
    MediaFile mediaFile_;
};

class AudioDecoderError : public std::runtime_error {
public:
    explicit AudioDecoderError(const std::string& errMessage) : std::runtime_error(errMessage) {}
};

#endif
