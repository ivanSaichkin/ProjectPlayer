#ifndef AUDIODECODER_HPP
#define AUDIODECODER_HPP

#include "Decoder.hpp"
#include "MediaFile.hpp"
#include "SFML/Audio.hpp"

class AudioDecoder : public Decoder {
 public:
    AudioDecoder(const MediaFile& mediaFile);
    ~AudioDecoder();
    void Start() override;
    void Stop() override;
    void SetVolume(float volume);
    void TogglePause() override;

 private:
    void DecodeAudio();

    AVCodecContext* audioCodecContext_;
    sf::Sound sound_;
    sf::SoundBuffer soundBuffer_;
    MediaFile mediaFile_;
};

#endif
