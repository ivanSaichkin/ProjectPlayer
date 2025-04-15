#pragma once

#include "MediaDecoder.hpp"
#include <SFML/Audio.hpp>
#include <queue>
#include <thread>
#include <atomic>
#include <mutex>

extern "C" {
#include <libswresample/swresample.h>
}

struct AudioPacket {
    std::vector<sf::Int16> samples;
    double pts;  // Presentation timestamp
};

class AudioDecoder : public MediaDecoder {
public:
    AudioDecoder();
    ~AudioDecoder() override;

    bool initialize();
    void start();
    void stop();

    // Get next audio packet
    bool getNextPacket(AudioPacket& packet);

    // Get audio parameters
    unsigned int getSampleRate() const;
    unsigned int getChannelCount() const;

    // Check if decoder has more packets
    bool hasMorePackets() const;

    // Pause/resume decoding
    void setPaused(bool paused);
    bool isPaused() const;

private:
    AVCodecContext* codecContext;
    SwrContext* swrContext;
    AVStream* audioStream;
    int audioStreamIndex;

    std::queue<AudioPacket> packetQueue;
    mutable std::mutex queueMutex;
    std::condition_variable queueCondition;

    std::thread decodingThread;
    std::atomic<bool> running;
    std::atomic<bool> paused;

    // Decoding thread function
    void decodingLoop();

    // Convert AVFrame to audio samples
    bool convertFrameToSamples(AVFrame* frame, std::vector<sf::Int16>& samples);
};
