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

void Player::Stop() {
    videoDecoder_->Stop();
    audioDecoder_->Stop();
}

void Player::Draw(sf::RenderWindow& window) {
    videoDecoder_->Draw(window);
}
