#ifndef FRAME_HPP
#define FRAME_HPP

#include <SFML/Graphics.hpp>

class Frame {
 public:
    Frame(double timestamp, const sf::Texture& texture);
    ~Frame() = default;

    double GetTimestamp() const;
    const sf::Texture& GetTexture() const;

 private:
    double timestamp_;
    sf::Texture texture_;
};

#endif
