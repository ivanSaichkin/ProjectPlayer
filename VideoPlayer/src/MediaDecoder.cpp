#include "../include/MediaDecoder.hpp"

#include <iostream>

MediaDecoder::MediaDecoder() : formatContext(nullptr), opened(false) {
    // Initialize FFmpeg if needed
    static bool ffmpegInitialized = false;
    if (!ffmpegInitialized) {
        av_log_set_level(AV_LOG_QUIET);
        ffmpegInitialized = true;
    }
}

MediaDecoder::~MediaDecoder() {
    close();
}

bool MediaDecoder::open(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex);

    // Close if already open
    if (opened) {
        close();
    }

    this->filename = filename;

    // Open input file
    formatContext = avformat_alloc_context();
    if (!formatContext) {
        ErrorHandler::getInstance().handleError(MediaPlayerException::DECODER_ERROR, "Failed to allocate format context");
        return false;
    }

    int result = avformat_open_input(&formatContext, filename.c_str(), nullptr, nullptr);
    if (result != 0) {
        ErrorHandler::getInstance().handleError(MediaPlayerException::FILE_NOT_FOUND,
                                                "Failed to open input file: " + filename + " - " + ErrorHandler::ffmpegErrorToString(result));
        avformat_free_context(formatContext);
        formatContext = nullptr;
        return false;
    }

    // Find stream info
    result = avformat_find_stream_info(formatContext, nullptr);
    if (result < 0) {
        ErrorHandler::getInstance().handleError(MediaPlayerException::FORMAT_ERROR,
                                                "Failed to find stream information: " + ErrorHandler::ffmpegErrorToString(result));
        avformat_close_input(&formatContext);
        formatContext = nullptr;
        return false;
    }

    opened = true;
    return true;
}

void MediaDecoder::close() {
    std::lock_guard<std::mutex> lock(mutex);

    if (formatContext) {
        avformat_close_input(&formatContext);
        formatContext = nullptr;
    }

    opened = false;
}

bool MediaDecoder::seek(double seconds) {
    std::lock_guard<std::mutex> lock(mutex);

    if (!opened || !formatContext) {
        ErrorHandler::getInstance().handleError(MediaPlayerException::DECODER_ERROR, "Cannot seek: decoder not open");
        return false;
    }

    int64_t timestamp = static_cast<int64_t>(seconds * AV_TIME_BASE);
    int result = av_seek_frame(formatContext, -1, timestamp, AVSEEK_FLAG_BACKWARD);

    if (result < 0) {
        ErrorHandler::getInstance().handleError(MediaPlayerException::DECODER_ERROR, "Failed to seek to position " + std::to_string(seconds) +
                                                                                         " seconds: " + ErrorHandler::ffmpegErrorToString(result));
        return false;
    }

    return true;
}

double MediaDecoder::getDuration() const {
    if (!opened || !formatContext) {
        return 0.0;
    }

    if (formatContext->duration == AV_NOPTS_VALUE) {
        return 0.0;
    }

    return static_cast<double>(formatContext->duration) / AV_TIME_BASE;
}

bool MediaDecoder::isOpen() const {
    return opened;
}

int MediaDecoder::findStream(AVMediaType type) const {
    if (!opened || !formatContext) {
        return -1;
    }

    for (unsigned int i = 0; i < formatContext->nb_streams; ++i) {
        if (formatContext->streams[i]->codecpar->codec_type == type) {
            return i;
        }
    }

    return -1;
}
