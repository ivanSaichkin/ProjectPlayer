#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/time.h>
#include <libavutil/imgutils.h>
}

#include <string>
#include <memory>
#include <mutex>
#include <condition_variable>

class MediaDecoder {
public:
    MediaDecoder();
    virtual ~MediaDecoder();

    bool open(const std::string& filename);
    void close();

    // Seek to a specific position in seconds
    bool seek(double seconds);

    // Get total duration in seconds
    double getDuration() const;

    // Check if media is opened
    bool isOpen() const;

protected:
    AVFormatContext* formatContext;
    bool opened;
    std::mutex mutex;

    // Find stream by type
    int findStream(AVMediaType type) const;
};
