#ifndef AUDIODECODER_HPP
#define AUDIODECODER_HPP

#include "Decoder.hpp"
#include "MediaFile.hpp"
#include "SFML/Audio.hpp"

class AudioDecoder : public Decoder {
 public:
    AudioDecoder(const MediaFile& mediaFile);
    void Start() override;
    void Stop() override;

 private:
    void DecodeAudio();

    AVCodecContext* audioCodecContext_;
    sf::Sound sound_;
    sf::SoundBuffer soundBuffer_;
    MediaFile mediaFile_;
};

#endif
