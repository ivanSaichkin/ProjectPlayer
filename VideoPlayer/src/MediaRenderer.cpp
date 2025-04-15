#include "../include/MediaRenderer.hpp"
#include <iostream>
#include <chrono>
#include <thread>

MediaRenderer::MediaRenderer(VideoDecoder& videoDecoder, AudioDecoder& audioDecoder)
    : videoDecoder(videoDecoder),
      audioDecoder(audioDecoder),
      running(false),
      paused(false),
      currentPosition(0.0) {
}

MediaRenderer::~MediaRenderer() {
    stop();
}

void MediaRenderer::initialize() {
    // Initialize video sprite
    sf::Vector2u videoSize = videoDecoder.getSize();
    if (videoSize.x > 0 && videoSize.y > 0) {
        currentTexture.create(videoSize.x, videoSize.y);
        videoSprite.setTexture(currentTexture);
    }

    // Initialize audio stream
    audioStream = std::make_unique<CustomAudioStream>(audioDecoder);
}

void MediaRenderer::start() {
    if (running) {
        return;
    }

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
}

void MediaRenderer::stop() {
    if (!running) {
        return;
    }

    running = false;
    paused = false;

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

void MediaRenderer::renderingLoop() {
    const double frameTime = 1.0 / videoDecoder.getFrameRate();
    auto lastFrameTime = std::chrono::high_resolution_clock::now();

    while (running) {
        if (paused) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

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
    }
}

// CustomAudioStream implementation
MediaRenderer::CustomAudioStream::CustomAudioStream(AudioDecoder& decoder)
    : audioDecoder(decoder) {
    // Initialize sound stream
    initialize(decoder.getChannelCount(), decoder.getSampleRate());
}

void MediaRenderer::CustomAudioStream::start() {
    play();
}

void MediaRenderer::CustomAudioStream::stop() {
    sf::SoundStream::stop();
}

bool MediaRenderer::CustomAudioStream::onGetData(Chunk& data) {
    const int BUFFER_SIZE = 4096;  // Number of samples

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
}

void MediaRenderer::CustomAudioStream::onSeek(sf::Time timeOffset) {
    // Not used, seeking is handled at a higher level
}
