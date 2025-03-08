#include "../include/Player.hpp"

Player::Player(){}

void Player::Load(const std::string& filename)  {
    mediaFile_.Load(filename);
    videoDecoder_ = std::make_unique<VideoDecoder>(mediaFile_);
    audioDecoder_ = std::make_unique<AudioDecoder>(mediaFile_);
}

void Player::Play() {
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
}

void Player::Draw(sf::RenderWindow& window) {
    videoDecoder_->Draw(window);
}

void Player::Seek(int seconds) {
    uint64_t timestamp = seconds * AV_TIME_BASE;

    av_seek_frame(mediaFile_.GetFormatContext(), -1, timestamp, AVSEEK_FLAG_BACKWARD);

    videoDecoder_->Stop();
    audioDecoder_->Stop();

    videoDecoder_->Start();
    audioDecoder_->Start();
}

void Player::SetVolume(float volume) {
    audioDecoder_->SetVolume(volume);
}
