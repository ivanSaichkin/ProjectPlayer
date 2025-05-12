#pragma once

#include <stdexcept>
#include <string>

namespace VideoPlayer {
namespace Core {

class MediaPlayerException : public std::runtime_error {
 public:
    enum ErrorCode {
        UNKNOWN_ERROR = 0,
        INITIALIZATION_ERROR,
        FILE_OPEN_ERROR,
        DECODER_ERROR,
        PLAYBACK_ERROR,
        SEEK_ERROR,
        AUDIO_ERROR,
        VIDEO_ERROR,
        CONFIGURATION_ERROR,
        PLAYLIST_ERROR,
        FILESYSTEM_ERROR
    };

    MediaPlayerException(ErrorCode code, const std::string& message) : std::runtime_error(message), errorCode(code) {}

    ErrorCode getErrorCode() const { return errorCode; }

 private:
    ErrorCode errorCode;
};

}  // namespace Core
}  // namespace VideoPlayer
