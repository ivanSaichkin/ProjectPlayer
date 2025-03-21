#include "../include/Player.hpp"

#include <iostream>

Player::Player() : timeOffset_(0.0) {
}

void Player::Load(const std::string& filename) {
    try {
        // Stop current playback if any
        Stop();

        // Load media file
        mediaFile_ = MediaFile();
        if (!mediaFile_.Load(filename)) {
            std::cerr << "Failed to load media file: " << filename << std::endl;
            return;
        }

        // Create decoders
        if (mediaFile_.GetVideoStreamIndex() >= 0) {
            videoDecoder_ = std::make_unique<VideoDecoder>(mediaFile_);
        }

        if (mediaFile_.GetAudioStreamIndex() >= 0) {
            audioDecoder_ = std::make_unique<AudioDecoder>(mediaFile_);
        }

        timeOffset_ = 0.0;

    } catch (const std::exception& e) {
        std::cerr << "Error loading media: " << e.what() << std::endl;
        videoDecoder_.reset();
        audioDecoder_.reset();
    }
}

void Player::Play() {
    if (!videoDecoder_ && !audioDecoder_) {
        std::cerr << "No media loaded" << std::endl;
        return;
    }

    auto offsetDurationSteady = std::chrono::milliseconds(static_cast<int>(timeOffset_ * 1000));
    startTime_ = std::chrono::steady_clock::now() - offsetDurationSteady;

    if (videoDecoder_) {
        videoDecoder_->SetStartTime(startTime_);
        videoDecoder_->Start();
    }

    if (audioDecoder_) {
        audioDecoder_->SetStartTime(startTime_);
        audioDecoder_->Start();
    }
}

void Player::TogglePause() {
    if (videoDecoder_) {
        videoDecoder_->TogglePause();
    }

    if (audioDecoder_) {
        audioDecoder_->TogglePause();
    }

    // Update time offset when pausing/resuming
    timeOffset_ = GetCurrentTime();
    auto offsetDurationSteady = std::chrono::milliseconds(static_cast<int>(timeOffset_ * 1000));
    startTime_ = std::chrono::steady_clock::now() - offsetDurationSteady;
}

void Player::Stop() {
    if (videoDecoder_) {
        videoDecoder_->Stop();
    }

    if (audioDecoder_) {
        audioDecoder_->Stop();
    }

    timeOffset_ = 0.0;
}

void Player::Draw(sf::RenderWindow& window) {
    if (videoDecoder_) {
        videoDecoder_->Draw(window);
    }
}

void Player::Seek(int seconds) {
    if (!mediaFile_.GetFormatContext()) {
        return;
    }

    // Calculate target position
    double currentTime = GetCurrentTime();
    double targetTime = currentTime + seconds;

    if (targetTime < 0) {
        targetTime = 0;
    } else if (targetTime > GetDuration()) {
        targetTime = GetDuration();
    }

    // Seek in the video
    int64_t timestamp = static_cast<int64_t>(targetTime / av_q2d(mediaFile_.GetFormatContext()->streams[0]->time_base));
    int result = av_seek_frame(mediaFile_.GetFormatContext(), -1, timestamp, seconds > 0 ? AVSEEK_FLAG_ANY : AVSEEK_FLAG_BACKWARD);

    if (result < 0) {
        std::cerr << "Error seeking: " << result << std::endl;
        return;
    }

    // Flush decoders
    if (videoDecoder_) {
        videoDecoder_->Flush();
    }

    if (audioDecoder_) {
        audioDecoder_->Flush();
    }

    // Update timing
    timeOffset_ = targetTime;
    auto offsetDurationSteady = std::chrono::milliseconds(static_cast<int>(timeOffset_ * 1000));
    startTime_ = std::chrono::steady_clock::now() - offsetDurationSteady;

    if (videoDecoder_) {
        videoDecoder_->SetStartTime(startTime_);
    }

    if (audioDecoder_) {
        audioDecoder_->SetStartTime(startTime_);
    }
}

void Player::SetVolume(float volume) {
    if (audioDecoder_) {
        audioDecoder_->SetVolume(volume);
    }
}

double Player::GetDuration() const {
    if (mediaFile_.GetFormatContext()) {
        return mediaFile_.GetFormatContext()->duration / (double)AV_TIME_BASE;
    }
    return 0.0;
}

double Player::GetCurrentTime() const {
    auto now = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(now - startTime_).count();
    return elapsed;
}
