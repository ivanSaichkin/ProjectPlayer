#ifndef MEDIAFILE_HPP
#define MEDIAFILE_HPP

#include <stdexcept>
#include <string>

extern "C" {
#include "libavformat/avformat.h"
}

class MediaFile {
 public:
    MediaFile();
    ~MediaFile();
    bool Load(const std::string& filename);
    std::string GetFilePath() const;
    int GetVideoStreamIndex() const;
    int GetAudioStreamIndex() const;
    AVFormatContext* GetFormatContext() const;

    double GetVideoTimeBase() const;
    double GetAudioTimeBase() const;

 private:
    std::string filePath_;
    AVFormatContext* formatContext_;
    int videoStreamIndex_;
    int audioStreamIndex_;
};

class MediaFileError : public std::runtime_error {
 public:
    explicit MediaFileError(const std::string& errMessage) : std::runtime_error(errMessage) {}
};

#endif
