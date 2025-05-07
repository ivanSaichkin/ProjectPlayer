#pragma once

#include <SFML/Audio.hpp>
#include <atomic>
#include <queue>
#include <thread>

#include "MediaDecoder.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
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

    // Initialize the audio decoder after a file is opened
    bool initialize();

    // Start the decoding thread
    void start();

    // Stop the decoding thread and clean up resources
    void stop();

    // Get next audio packet
    bool getNextPacket(AudioPacket& packet);

    // Get audio properties
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
    std::mutex queueMutex;
    std::condition_variable queueCondition;

    std::thread decodingThread;
    std::atomic<bool> running;
    std::atomic<bool> paused;

    // Maximum number of packets to keep in queue
    static constexpr size_t MAX_QUEUE_SIZE = 100;

    // Decoding thread function
    void decodingLoop();

    // Convert AVFrame to audio samples
    bool convertFrameToSamples(AVFrame* frame, std::vector<sf::Int16>& samples);
};
