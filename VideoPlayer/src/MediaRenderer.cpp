#include "../include/MediaRenderer.hpp"

#include <chrono>
#include <iostream>
#include <thread>

#include "../include/ErrorHandler.hpp"

MediaRenderer::MediaRenderer(VideoDecoder& videoDecoder, AudioDecoder& audioDecoder)
    : videoDecoder(videoDecoder), audioDecoder(audioDecoder), running(false), paused(false), currentPosition(0.0), volume(1.0f) {
}

MediaRenderer::~MediaRenderer() {
    stop();
}

bool MediaRenderer::initialize() {
    try {
        // Initialize video sprite
        sf::Vector2u videoSize = videoDecoder.getSize();
        if (videoSize.x > 0 && videoSize.y > 0) {
            if (!currentTexture.create(videoSize.x, videoSize.y)) {
                ErrorHandler::getInstance().handleError(
                    MediaPlayerException::RENDER_ERROR,
                    "Failed to create video texture with size " + std::to_string(videoSize.x) + "x" + std::to_string(videoSize.y));
                return false;
            }
            videoSprite.setTexture(currentTexture);
        } else {
            ErrorHandler::getInstance().handleError(MediaPlayerException::RENDER_ERROR,
                                                    "Invalid video dimensions: " + std::to_string(videoSize.x) + "x" + std::to_string(videoSize.y));
            return false;
        }

        // Initialize audio stream
        audioStream = std::make_unique<CustomAudioStream>(audioDecoder);
        audioStream->setVolumeLevel(volume * 100.0f);  // SFML uses 0-100 scale

        return true;
    } catch (const std::exception& e) {
        ErrorHandler::getInstance().handleError(MediaPlayerException::RENDER_ERROR,
                                                std::string("Exception during renderer initialization: ") + e.what());
        return false;
    }
}

void MediaRenderer::start() {
    if (running) {
        return;
    }

    try {
        running = true;
        paused = false;

        // Start decoders
        videoDecoder.start();
        audioDecoder.start();

        // Start audio playback
        if (audioStream) {
            audioStream->start();
        }

        // Start rendering thread
        renderThread = std::thread(&MediaRenderer::renderingLoop, this);
    } catch (const std::exception& e) {
        ErrorHandler::getInstance().handleError(MediaPlayerException::RENDER_ERROR, std::string("Exception during renderer start: ") + e.what());
        stop();
    }
}

void MediaRenderer::stop() {
    if (!running) {
        return;
    }

    running = false;
    paused = false;

    try {
        // Stop audio playback
        if (audioStream) {
            audioStream->stop();
        }

        // Stop decoders
        videoDecoder.stop();
        audioDecoder.stop();

        // Stop rendering thread
        if (renderThread.joinable()) {
            renderThread.join();
        }
    } catch (const std::exception& e) {
        ErrorHandler::getInstance().handleError(MediaPlayerException::RENDER_ERROR, std::string("Exception during renderer stop: ") + e.what());
    }
}

void MediaRenderer::render(sf::RenderTarget& target) {
    target.draw(videoSprite);
}

double MediaRenderer::getCurrentPosition() const {
    return currentPosition;
}

void MediaRenderer::setPaused(bool pause) {
    paused = pause;
    videoDecoder.setPaused(pause);
    audioDecoder.setPaused(pause);

    if (audioStream) {
        if (pause) {
            audioStream->pause();
        } else {
            audioStream->play();
        }
    }
}

bool MediaRenderer::isPaused() const {
    return paused;
}

void MediaRenderer::setVolume(float newVolume) {
    volume = std::max(0.0f, std::min(newVolume, 1.0f));

    if (audioStream) {
        audioStream->setVolumeLevel(volume * 100.0f);  // SFML uses 0-100 scale
    }
}

float MediaRenderer::getVolume() const {
    return volume;
}

void MediaRenderer::renderingLoop() {
    const double frameTime = 1.0 / std::max(videoDecoder.getFrameRate(), 1.0);
    auto lastFrameTime = std::chrono::high_resolution_clock::now();

    while (running) {
        if (paused) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        try {
            VideoFrame frame;
            if (videoDecoder.getNextFrame(frame)) {
                // Update current texture
                currentTexture = frame.texture;
                videoSprite.setTexture(currentTexture, true);

                // Update current position
                currentPosition = frame.pts;

                // Calculate time to wait for next frame
                auto now = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> elapsed = now - lastFrameTime;
                double sleepTime = frameTime - elapsed.count();

                if (sleepTime > 0) {
                    std::this_thread::sleep_for(std::chrono::duration<double>(sleepTime));
                }

                lastFrameTime = std::chrono::high_resolution_clock::now();
            } else {
                // No frame available, sleep a bit
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        } catch (const std::exception& e) {
            ErrorHandler::getInstance().handleError(MediaPlayerException::RENDER_ERROR, std::string("Exception in rendering loop: ") + e.what());
            // Continue rendering, don't break the loop
        }
    }
}

// CustomAudioStream implementation
MediaRenderer::CustomAudioStream::CustomAudioStream(AudioDecoder& decoder) : audioDecoder(decoder), volumeLevel(100.0f) {
    // Initialize sound stream
    initialize(decoder.getChannelCount(), decoder.getSampleRate());
}

void MediaRenderer::CustomAudioStream::start() {
    play();
}

void MediaRenderer::CustomAudioStream::stop() {
    sf::SoundStream::stop();
}

void MediaRenderer::CustomAudioStream::setVolumeLevel(float level) {
    volumeLevel = std::max(0.0f, std::min(level, 100.0f));
    sf::SoundStream::setVolume(volumeLevel);
}

float MediaRenderer::CustomAudioStream::getVolumeLevel() const {
    return volumeLevel;
}

bool MediaRenderer::CustomAudioStream::onGetData(Chunk& data) {
    const int BUFFER_SIZE = 4096;  // Number of samples

    try {
        // Ensure buffer is big enough
        if (buffer.size() < BUFFER_SIZE) {
            buffer.resize(BUFFER_SIZE);
        }

        // Fill buffer with audio samples
        int samplesRead = 0;
        while (samplesRead < BUFFER_SIZE) {
            AudioPacket packet;
            if (audioDecoder.getNextPacket(packet)) {
                int samplesToRead = std::min(static_cast<int>(packet.samples.size()), BUFFER_SIZE - samplesRead);
                std::copy(packet.samples.begin(), packet.samples.begin() + samplesToRead, buffer.begin() + samplesRead);
                samplesRead += samplesToRead;
            } else {
                // No more packets available
                break;
            }
        }

        if (samplesRead == 0) {
            // No samples read, end of stream
            return false;
        }

        // Set data pointer and size
        data.samples = buffer.data();
        data.sampleCount = samplesRead;

        return true;
    } catch (const std::exception& e) {
        ErrorHandler::getInstance().handleError(MediaPlayerException::RENDER_ERROR, std::string("Exception in audio streaming: ") + e.what());
        return false;
    }
}

void MediaRenderer::CustomAudioStream::onSeek(sf::Time timeOffset) {
    // Not used, seeking is handled at a higher level
}
