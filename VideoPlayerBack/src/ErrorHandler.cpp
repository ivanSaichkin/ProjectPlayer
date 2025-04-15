#include "../include/ErrorHandler.hpp"

#include <iostream>

extern "C" {
#include <libavutil/error.h>
}

ErrorHandler& ErrorHandler::getInstance() {
    static ErrorHandler instance;
    return instance;
}

void ErrorHandler::setErrorCallback(std::function<void(const MediaPlayerException&)> callback) {
    errorCallback = std::move(callback);
}

void ErrorHandler::handleError(MediaPlayerException::ErrorCode code, const std::string& message) {
    MediaPlayerException exception(code, message);

    // Always log the error
    std::cerr << "Error: " << message << std::endl;

    // Call the callback if set
    if (errorCallback) {
        errorCallback(exception);
    }
}

std::string ErrorHandler::ffmpegErrorToString(int errorCode) {
    char errorStr[AV_ERROR_MAX_STRING_SIZE] = {0};
    av_strerror(errorCode, errorStr, AV_ERROR_MAX_STRING_SIZE);
    return std::string(errorStr);
}
