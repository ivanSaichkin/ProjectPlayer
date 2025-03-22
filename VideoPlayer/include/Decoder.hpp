#ifndef DECODER_HPP
#define DECODER_HPP

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
}

class Decoder {
 public:
    virtual ~Decoder() = default;

    virtual void Start() = 0;
    void Stop();
    void SetPaused(bool paused);
    virtual void Flush() = 0;
    void SetStartTime(std::chrono::steady_clock::time_point startTime);
    virtual void ProcessPacket(AVPacket* packet) = 0;
    virtual void SignalEndOfStream() = 0;

 protected:
    std::atomic<bool> isRunning_;
    std::atomic<bool> isPaused_;
    std::mutex mutex_;
    std::chrono::steady_clock::time_point startTime_;
    std::queue<AVPacket*> packetQueue_;
    std::mutex queueMutex_;
    std::condition_variable packetCondition_;
    std::atomic<bool> endOfStream_;
};

#endif
