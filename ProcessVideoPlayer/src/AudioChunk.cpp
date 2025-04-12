#include "../include/AudioChunk.hpp"

AudioChunk::AudioChunk(double timestamp, const std::vector<int16_t>& samples, unsigned int sampleRate, unsigned int channels)
    : timestamp_(timestamp), samples_(samples), sampleRate_(sampleRate), channels_(channels) {
}

double AudioChunk::GetTimestamp() const {
    return timestamp_;
}

const std::vector<int16_t>& AudioChunk::GetSamples() const {
    return samples_;
}

unsigned int AudioChunk::GetSampleRate() const {
    return sampleRate_;
}

unsigned int AudioChunk::GetChannels() const {
    return channels_;
}

double AudioChunk::GetDuration() const {
    // Вычисляем длительность аудиочанка в секундах
    if (sampleRate_ == 0 || channels_ == 0)
        return 0.0;
    return static_cast<double>(samples_.size()) / (sampleRate_ * channels_);
}
