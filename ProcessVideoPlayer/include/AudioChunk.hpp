#ifndef AUDIOCHUNK_HPP
#define AUDIOCHUNK_HPP

#include <SFML/Audio.hpp>
#include <vector>

class AudioChunk {
 public:
    AudioChunk(double timestamp, const std::vector<int16_t>& samples, unsigned int sampleRate, unsigned int channels);
    ~AudioChunk() = default;

    double GetTimestamp() const;
    const std::vector<int16_t>& GetSamples() const;
    unsigned int GetSampleRate() const;
    unsigned int GetChannels() const;
    double GetDuration() const;

 private:
    double timestamp_;
    std::vector<int16_t> samples_;
    unsigned int sampleRate_;
    unsigned int channels_;
};

#endif
