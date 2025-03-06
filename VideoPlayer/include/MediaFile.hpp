#ifndef MEDIAFILE_HPP
#define MEDIAFILE_HPP

#include <string>
#include <stdexcept>
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

private:
    std::string filePath_;
    AVFormatContext* formatContext_;
    int videoStreamIndex_;
    int audioStreamIndex_;
};

#endif
