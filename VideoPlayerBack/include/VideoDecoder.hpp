#pragma once

#include <SFML/Graphics.hpp>
#include <atomic>
#include <queue>
#include <thread>

#include "MediaDecoder.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
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

    // Initialize the video decoder after a file is opened
    bool initialize();

    // Start the decoding thread
    void start();

    // Stop the decoding thread and clean up resources
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
    std::mutex queueMutex;
    std::condition_variable queueCondition;

    std::thread decodingThread;
    std::atomic<bool> running;
    std::atomic<bool> paused;

    // Maximum number of frames to keep in queue
    static constexpr size_t MAX_QUEUE_SIZE = 30;

    // Decoding thread function
    void decodingLoop();

    // Convert AVFrame to SFML Texture
    bool convertFrameToTexture(AVFrame* frame, sf::Texture& texture);
};
