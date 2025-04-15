#pragma once

#include <string>
#include <exception>
#include <functional>

// Custom exception class for media player errors
class MediaPlayerException : public std::exception {
public:
    enum ErrorCode {
        FILE_NOT_FOUND,
        DECODER_ERROR,
        FORMAT_ERROR,
        CODEC_ERROR,
        STREAM_ERROR,
        RENDER_ERROR,
        UNKNOWN_ERROR
    };

    MediaPlayerException(ErrorCode code, const std::string& message)
        : code(code), message(message) {}

    const char* what() const noexcept override {
        return message.c_str();
    }

    ErrorCode getCode() const {
        return code;
    }

private:
    ErrorCode code;
    std::string message;
};

class ErrorHandler {
public:
    // Singleton pattern
    static ErrorHandler& getInstance();

    // Set error callback
    void setErrorCallback(std::function<void(const MediaPlayerException&)> callback);

    // Handle error and call callback if set
    void handleError(MediaPlayerException::ErrorCode code, const std::string& message);

    // Convert FFmpeg error code to string
    static std::string ffmpegErrorToString(int errorCode);

private:
    ErrorHandler() = default;
    ~ErrorHandler() = default;
    ErrorHandler(const ErrorHandler&) = delete;
    ErrorHandler& operator=(const ErrorHandler&) = delete;

    std::function<void(const MediaPlayerException&)> errorCallback;
};

// Macro for error checking with FFmpeg functions
#define CHECK_FFMPEG_ERROR(result, operation) \
    if (result < 0) { \
        ErrorHandler::getInstance().handleError( \
            MediaPlayerException::DECODER_ERROR, \
            std::string("FFmpeg error: ") + operation + " failed: " + \
            ErrorHandler::ffmpegErrorToString(result) \
        ); \
        return false; \
    }
