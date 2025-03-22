#include "../include/MediaFile.hpp"

#include <iostream>

MediaFile::MediaFile() : formatContext_(nullptr), videoStreamIndex_(-1), audioStreamIndex_(-1) {
}

MediaFile::~MediaFile() {
    if (formatContext_) {
        avformat_close_input(&formatContext_);
    }
}

bool MediaFile::Load(const std::string& filename) {
    filePath_ = filename;

    // Open input file
    formatContext_ = avformat_alloc_context();
    if (!formatContext_) {
        throw MediaFileError("Could not allocate format context");
    }

    if (avformat_open_input(&formatContext_, filename.c_str(), nullptr, nullptr) != 0) {
        throw MediaFileError("Could not open input file: " + filename);
    }

    // Find stream information
    if (avformat_find_stream_info(formatContext_, nullptr) < 0) {
        throw MediaFileError("Could not find stream information");
    }

    // Find video and audio streams
    videoStreamIndex_ = -1;
    audioStreamIndex_ = -1;

    for (unsigned int i = 0; i < formatContext_->nb_streams; i++) {
        AVStream* stream = formatContext_->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && videoStreamIndex_ < 0) {
            videoStreamIndex_ = i;
        } else if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audioStreamIndex_ < 0) {
            audioStreamIndex_ = i;
        }
    }

    if (videoStreamIndex_ == -1 && audioStreamIndex_ == -1) {
        throw MediaFileError("No audio or video streams found");
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
    if (videoStreamIndex_ >= 0) {
        AVStream* stream = formatContext_->streams[videoStreamIndex_];
        return av_q2d(stream->time_base);
    }
    return 0.0;
}

double MediaFile::GetAudioTimeBase() const {
    if (audioStreamIndex_ >= 0) {
        AVStream* stream = formatContext_->streams[audioStreamIndex_];
        return av_q2d(stream->time_base);
    }
    return 0.0;
}
