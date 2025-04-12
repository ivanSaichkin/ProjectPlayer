#include "../include/AudioManager.hpp"
#include <algorithm>
#include <iostream>

AudioManager::AudioManager()
    : currentChunkIndex_(0),
      currentSampleIndex_(0),
      volume_(100.0f) {
    // Устанавливаем параметры потока по умолчанию
    // Они будут обновлены при добавлении первого чанка
    initialize(2, 44100);
}

AudioManager::~AudioManager() {
    stop();
    Clear();
}

void AudioManager::AddChunk(std::unique_ptr<AudioChunk> chunk) {
    stop();

    std::lock_guard<std::mutex> lock(mutex_);

    // Если это первый чанк, инициализируем поток
    if (chunks_.empty()) {
        initialize(chunk->GetChannels(), chunk->GetSampleRate());
    }

    chunks_.push_back(std::move(chunk));
}

void AudioManager::SetCurrentTime(double time) {
    stop();

    std::lock_guard<std::mutex> lock(mutex_);

    if (chunks_.empty()) {
        return;
    }

    // Находим чанк для указанного времени
    currentChunkIndex_ = FindChunkIndex(time);

    if (currentChunkIndex_ >= chunks_.size()) {
        currentChunkIndex_ = chunks_.size() - 1;
        currentSampleIndex_ = chunks_[currentChunkIndex_]->GetSamples().size();
        return;
    }

    // Вычисляем индекс семпла внутри чанка
    const AudioChunk* chunk = chunks_[currentChunkIndex_].get();
    double chunkOffset = time - chunk->GetTimestamp();
    if (chunkOffset < 0) chunkOffset = 0;

    currentSampleIndex_ = static_cast<size_t>(chunkOffset * chunk->GetSampleRate() * chunk->GetChannels());
    if (currentSampleIndex_ >= chunk->GetSamples().size()) {
        currentSampleIndex_ = 0;
        if (currentChunkIndex_ < chunks_.size() - 1) {
            currentChunkIndex_++;
        }
    }

    // Устанавливаем позицию в потоке
    sf::Time seekTime = sf::seconds(static_cast<float>(time));
    onSeek(seekTime);
}

double AudioManager::GetDuration() const {
    std::lock_guard<std::mutex> lock(mutex_);

    if (chunks_.empty()) {
        return 0.0;
    }

    const AudioChunk* lastChunk = chunks_.back().get();
    return lastChunk->GetTimestamp() + lastChunk->GetDuration();
}

void AudioManager::Clear() {
    stop();

    std::lock_guard<std::mutex> lock(mutex_);
    chunks_.clear();
    currentBuffer_.clear();
    currentChunkIndex_ = 0;
    currentSampleIndex_ = 0;
}

void AudioManager::Play() {
    sf::SoundStream::play();
}

void AudioManager::Pause() {
    sf::SoundStream::pause();
}

void AudioManager::Stop() {
    sf::SoundStream::stop();
}

void AudioManager::SetVolume(float volume) {
    volume_ = std::clamp(volume, 0.0f, 100.0f);
    sf::SoundStream::setVolume(volume_);
}

float AudioManager::GetVolume() const {
    return volume_;
}

bool AudioManager::onGetData(Chunk& data) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (chunks_.empty() || currentChunkIndex_ >= chunks_.size()) {
        return false;
    }

    // Размер буфера для передачи SFML (примерно 100мс звука)
    const size_t BUFFER_SIZE = 4096;
    currentBuffer_.clear();
    currentBuffer_.reserve(BUFFER_SIZE);

    while (currentBuffer_.size() < BUFFER_SIZE && currentChunkIndex_ < chunks_.size()) {
        const AudioChunk* chunk = chunks_[currentChunkIndex_].get();
        const std::vector<int16_t>& samples = chunk->GetSamples();

        // Копируем семплы из текущего чанка
        size_t samplesLeft = samples.size() - currentSampleIndex_;
        size_t samplesToCopy = std::min(samplesLeft, BUFFER_SIZE - currentBuffer_.size());

        currentBuffer_.insert(currentBuffer_.end(),
                             samples.begin() + currentSampleIndex_,
                             samples.begin() + currentSampleIndex_ + samplesToCopy);

        currentSampleIndex_ += samplesToCopy;

        // Если чанк закончился, переходим к следующему
        if (currentSampleIndex_ >= samples.size()) {
            currentChunkIndex_++;
            currentSampleIndex_ = 0;
        }
    }

    // Если буфер пуст, значит воспроизведение завершено
    if (currentBuffer_.empty()) {
        return false;
    }

    // Передаем данные SFML
    data.samples = currentBuffer_.data();
    data.sampleCount = currentBuffer_.size();

    return true;
}

void AudioManager::onSeek(sf::Time timeOffset) {
    // SFML вызывает этот метод при вызове seek()
    // Мы уже установили необходимые индексы в SetCurrentTime, поэтому здесь ничего не делаем
}

size_t AudioManager::FindChunkIndex(double time) const {
    // Бинарный поиск для нахождения чанка по времени
    auto it = std::lower_bound(chunks_.begin(), chunks_.end(), time,
        [](const std::unique_ptr<AudioChunk>& chunk, double t) {
            return chunk->GetTimestamp() + chunk->GetDuration() < t;
        });

    if (it == chunks_.end()) {
        return chunks_.size() - 1;
    }

    return std::distance(chunks_.begin(), it);
}
