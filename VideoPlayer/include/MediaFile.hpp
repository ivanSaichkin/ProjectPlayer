#ifndef MEDIAFILE_HPP
#define MEDIAFILE_HPP

#include <stdexcept>
#include <string>

#include "libavformat/avformat.h"

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

#endif
