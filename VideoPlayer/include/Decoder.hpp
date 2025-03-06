#ifndef DECODER_HPP
#define DECODER_HPP

#include <atomic>
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

protected:
    std::atomic<bool> isRunning_;
    std::mutex mutex_;
};


#endif
