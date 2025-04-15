#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
}

#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>

#include "ErrorHandler.hpp"

class MediaDecoder {
 public:
    MediaDecoder();
    virtual ~MediaDecoder();

    // Open a media file for decoding
    bool open(const std::string& filename);

    // Close the media file and release resources
    void close();

    // Seek to a specific position in seconds
    bool seek(double seconds);

    // Get the total duration of the media in seconds
    double getDuration() const;

    // Check if the media is currently open
    bool isOpen() const;

 protected:
    // Find a stream of the specified type
    int findStream(AVMediaType type) const;

    AVFormatContext* formatContext;
    bool opened;
    std::mutex mutex;
    std::string filename;
};
