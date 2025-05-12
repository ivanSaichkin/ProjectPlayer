#pragma once

#include <functional>
#include <string>
#include <vector>

#include "MediaPlayerException.hpp"

namespace VideoPlayer {
namespace Core {

class ErrorHandler {
 public:
    // Singleton pattern
    static ErrorHandler& getInstance();

    // Error handling
    void handleError(const MediaPlayerException& error);
    void handleError(MediaPlayerException::ErrorCode code, const std::string& message);

    // Error callback registration
    void setErrorCallback(std::function<void(const MediaPlayerException&)> callback);

    // Error log
    struct ErrorLogEntry {
        MediaPlayerException::ErrorCode errorCode;
        std::string message;
        std::string timestamp;
    };

    std::vector<ErrorLogEntry> getErrorLog() const;
    void clearErrorLog();

 private:
    ErrorHandler();
    ~ErrorHandler();
    ErrorHandler(const ErrorHandler&) = delete;
    ErrorHandler& operator=(const ErrorHandler&) = delete;

    std::function<void(const MediaPlayerException&)> errorCallback;
    std::vector<ErrorLogEntry> errorLog;

    // Log error to file
    void logErrorToFile(const MediaPlayerException& error);
};

}  // namespace Core
}  // namespace VideoPlayer
