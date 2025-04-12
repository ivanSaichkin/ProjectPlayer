#include "../include/Frame.hpp"

Frame::Frame(double timestamp, const sf::Texture& texture) : timestamp_(timestamp), texture_(texture) {
}

double Frame::GetTimestamp() const {
    return timestamp_;
}

const sf::Texture& Frame::GetTexture() const {
    return texture_;
}
