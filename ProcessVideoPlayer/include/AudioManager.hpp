#ifndef AUDIOMANAGER_HPP
#define AUDIOMANAGER_HPP

#include <SFML/Audio.hpp>
#include <memory>
#include <mutex>
#include <vector>

#include "AudioChunk.hpp"

class AudioManager : public sf::SoundStream {
 public:
    AudioManager();
    ~AudioManager();

    void AddChunk(std::unique_ptr<AudioChunk> chunk);
    void SetCurrentTime(double time);
    double GetDuration() const;
    void Clear();

    // Реализация интерфейса sf::SoundStream
    void Play();
    void Pause();
    void Stop();
    void SetVolume(float volume);
    float GetVolume() const;

 private:
    // Методы, требуемые sf::SoundStream
    virtual bool onGetData(Chunk& data) override;
    virtual void onSeek(sf::Time timeOffset) override;

    // Внутренние методы
    void ProcessChunks();
    size_t FindChunkIndex(double time) const;

    std::vector<std::unique_ptr<AudioChunk>> chunks_;
    mutable std::mutex mutex_;
    std::vector<int16_t> currentBuffer_;
    size_t currentChunkIndex_;
    size_t currentSampleIndex_;
    float volume_;
};

#endif
