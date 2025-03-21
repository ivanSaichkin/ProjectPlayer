#ifndef DECODER_HPP
#define DECODER_HPP

#include <atomic>
#include <chrono>
#include <mutex>
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
    void TogglePause();
    virtual void Flush() = 0;
    void SetStartTime(std::chrono::steady_clock::time_point startTime);

 protected:
    std::atomic<bool> isRunning_;
    std::atomic<bool> isPaused_;
    std::mutex mutex_;
    std::chrono::steady_clock::time_point startTime_;
};

#endif
