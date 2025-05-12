#include "../../include/core/ErrorHandler.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>

#include "../../include/storage/FileManager.hpp"

namespace fs = std::filesystem;
namespace VideoPlayer {
namespace Core {

ErrorHandler& ErrorHandler::getInstance() {
    static ErrorHandler instance;
    return instance;
}

ErrorHandler::ErrorHandler() {
    // Initialize error log
}

ErrorHandler::~ErrorHandler() {
    // Clean up if needed
}

void ErrorHandler::handleError(const MediaPlayerException& error) {
    // Create error log entry
    ErrorLogEntry entry;
    entry.errorCode = error.getErrorCode();
    entry.message = error.what();

    // Add timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    entry.timestamp = ss.str();

    // Add to log
    errorLog.push_back(entry);

    // Log to file
    logErrorToFile(error);

    // Call callback if registered
    if (errorCallback) {
        errorCallback(error);
    }
}

void ErrorHandler::handleError(MediaPlayerException::ErrorCode code, const std::string& message) {
    handleError(MediaPlayerException(code, message));
}

void ErrorHandler::setErrorCallback(std::function<void(const MediaPlayerException&)> callback) {
    errorCallback = std::move(callback);
}

std::vector<ErrorHandler::ErrorLogEntry> ErrorHandler::getErrorLog() const {
    return errorLog;
}

void ErrorHandler::clearErrorLog() {
    errorLog.clear();
}

void ErrorHandler::logErrorToFile(const MediaPlayerException& error) {
    try {
        // Get log directory
        std::string logDir = Storage::FileManager::getInstance().getDataDirectory() + "/logs";

        // Create directory if it doesn't exist
        if (!fs::exists(logDir)) {
            fs::create_directories(logDir);
        }

        // Create log file name with date
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d");
        std::string logFile = logDir + "/error_" + ss.str() + ".log";

        // Open log file in append mode
        std::ofstream file(logFile, std::ios::app);
        if (file.is_open()) {
            // Write timestamp
            ss.str("");
            ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");

            // Write error details
            file << "[" << ss.str() << "] " << "Error code: " << error.getErrorCode() << " - " << error.what() << std::endl;

            file.close();
        }
    } catch (const std::exception&) {
        // Silently fail if we can't log the error
        // We don't want to create an infinite loop of errors
    }
}

}  // namespace Core
}  // namespace VideoPlayer
