#ifndef DECODER_HPP
#define DECODER_HPP

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"

class Decoder {
 public:
    virtual ~Decoder() = default;

    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual void TogglePause() = 0;
    virtual void Flush() = 0;
    virtual void SetStartTime(std::chrono::steady_clock::time_point startTime) = 0;

 protected:
    std::atomic<bool> isRunning_;
    std::atomic<bool> isPaused_;
    std::mutex mutex_;
    std::chrono::steady_clock::time_point startTime_;
};

#endif
