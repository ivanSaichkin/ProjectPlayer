#include "../include/Player.hpp"

Player::Player() : timeOffset_(0) {
}

void Player::Load(const std::string& filename) {
    mediaFile_.Load(filename);
    videoDecoder_ = std::make_unique<VideoDecoder>(mediaFile_);
    audioDecoder_ = std::make_unique<AudioDecoder>(mediaFile_);
}

void Player::Play() {
    startTime_ = std::chrono::steady_clock::now();

    videoDecoder_->SetStartTime(startTime_);
    audioDecoder_->SetStartTime(startTime_);

    videoDecoder_->Start();
    audioDecoder_->Start();
}

void Player::TogglePause() {
    videoDecoder_->TogglePause();
    audioDecoder_->TogglePause();
}

void Player::Stop() {
    videoDecoder_->Stop();
    audioDecoder_->Stop();
    timeOffset_ = 0;
}

void Player::Draw(sf::RenderWindow& window) {
    videoDecoder_->Draw(window);
}

void Player::Seek(int seconds) {
    double videoTimeBase = mediaFile_.GetVideoTimeBase();
    double audioTimeBase = mediaFile_.GetAudioTimeBase();

    int64_t targetTimestamp = seconds * AV_TIME_BASE;

    av_seek_frame(mediaFile_.GetFormatContext(), mediaFile_.GetVideoStreamIndex(), targetTimestamp / (videoTimeBase * AV_TIME_BASE),
                  AVSEEK_FLAG_BACKWARD);

    av_seek_frame(mediaFile_.GetFormatContext(), mediaFile_.GetAudioStreamIndex(), targetTimestamp / (videoTimeBase * AV_TIME_BASE),
                  AVSEEK_FLAG_BACKWARD);

    videoDecoder_->Flush();
    audioDecoder_->Flush();
}

void Player::SetVolume(float volume) {
    audioDecoder_->SetVolume(volume);
}

double Player::GetDuration() const {
    return mediaFile_.GetFormatContext()->duration / static_cast<double>(AV_TIME_BASE);
}

double Player::GetCurrentTime() const {
    auto elapsed = std::chrono::steady_clock::now() - startTime_;
    return timeOffset_ + std::chrono::duration<double>(elapsed).count();
}
