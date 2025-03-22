#ifndef AUDIODECODER_HPP
#define AUDIODECODER_HPP

#include <queue>
#include <vector>

#include "Decoder.hpp"
#include "MediaFile.hpp"
#include "SFML/Audio.hpp"

class AudioDecoder : public Decoder {
 public:
    AudioDecoder(const MediaFile& mediaFile);
    ~AudioDecoder();
    void SetVolume(float volume);
    float GetVolume() const;

    void Start() override;
    void Flush() override;
    void ProcessPacket(AVPacket* packet) override;
    void SignalEndOfStream() override;

 private:
    void DecodeAudio();
    void ProcessAudioFrame(AVFrame* frame);

    AVCodecContext* audioCodecContext_;
    sf::Sound sound_;
    sf::SoundBuffer soundBuffer_;
    MediaFile mediaFile_;
    std::vector<int16_t> audioBuffer_;
    float volume_;
};

class AudioDecoderError : public std::runtime_error {
 public:
    explicit AudioDecoderError(const std::string& errMessage) : std::runtime_error(errMessage) {}
};

#endif
