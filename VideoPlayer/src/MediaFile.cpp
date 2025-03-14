#include "../include/MediaFile.hpp"

MediaFile::MediaFile() : formatContext_(nullptr), videoStreamIndex_(-1), audioStreamIndex_(-1) {
}

MediaFile::~MediaFile() {
    if (formatContext_) {
        avformat_close_input(&formatContext_);
    }
}

bool MediaFile::Load(const std::string& filename) {
    this->filePath_ = filename;

    if (avformat_open_input(&formatContext_, filePath_.c_str(), nullptr, nullptr) != 0) {
        throw std::runtime_error("Couldn't open");
    }

    if (avformat_find_stream_info(formatContext_, nullptr) < 0) {
        throw std::runtime_error("Couldn't find stream info");
    }

    for (size_t i = 0; i < formatContext_->nb_streams; ++i) {
        if (formatContext_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex_ = i;
        } else if (formatContext_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStreamIndex_ = i;
        }
    }

    if (videoStreamIndex_ == -1) {
        throw std::runtime_error("Couldn't find a video stream");
    }

    return true;
}

std::string MediaFile::GetFilePath() const {
    return filePath_;
}

int MediaFile::GetVideoStreamIndex() const {
    return videoStreamIndex_;
}

int MediaFile::GetAudioStreamIndex() const {
    return audioStreamIndex_;
}

AVFormatContext* MediaFile::GetFormatContext() const {
    return formatContext_;
}

double MediaFile::GetVideoTimeBase() const {
    return av_q2d(formatContext_->streams[videoStreamIndex_]->time_base);
}

double MediaFile::GetAudioTimeBase() const {
    return av_q2d(formatContext_->streams[audioStreamIndex_]->time_base);
}
