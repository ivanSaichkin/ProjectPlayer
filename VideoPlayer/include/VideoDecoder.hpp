#pragma once

#include "MediaDecoder.hpp"
#include <SFML/Graphics.hpp>
#include <queue>
#include <thread>
#include <atomic>

extern "C" {
#include <libswscale/swscale.h>
}

struct VideoFrame {
    sf::Texture texture;
    double pts;  // Presentation timestamp
};

class VideoDecoder : public MediaDecoder {
public:
    VideoDecoder();
    ~VideoDecoder() override;

    bool initialize();
    void start();
    void stop();

    // Get next video frame
    bool getNextFrame(VideoFrame& frame);

    // Get video dimensions
    sf::Vector2u getSize() const;

    // Get frame rate
    double getFrameRate() const;

    // Check if decoder has more frames
    bool hasMoreFrames() const;

    // Pause/resume decoding
    void setPaused(bool paused);
    bool isPaused() const;

private:
    AVCodecContext* codecContext;
    SwsContext* swsContext;
    AVStream* videoStream;
    int videoStreamIndex;

    std::queue<VideoFrame> frameQueue;
    mutable std::mutex queueMutex;
    std::condition_variable queueCondition;

    std::thread decodingThread;
    std::atomic<bool> running;
    std::atomic<bool> paused;

    // Decoding thread function
    void decodingLoop();

    // Convert AVFrame to SFML Texture
    bool convertFrameToTexture(AVFrame* frame, sf::Texture& texture);
};
