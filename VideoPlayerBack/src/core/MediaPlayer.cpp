#include "../../include/core/MediaPlayer.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>

namespace VideoPlayer {
namespace Core {

MediaPlayer::MediaPlayer()
    : playing(false), paused(false), currentPosition(0.0), duration(0.0), volume(100.0f), frameTime(0.0), threadRunning(false) {
    // Initialize decoders
    audioDecoder = std::make_unique<AudioDecoder>();
    videoDecoder = std::make_unique<VideoDecoder>();
}

MediaPlayer::~MediaPlayer() {
    // Stop playback and clean up
    stop();
    close();
}

bool MediaPlayer::open(const std::string& filePath) {
    try {
        // Stop playback if running
        stop();

        // Close previous file if open
        close();

        // Initialize video decoder
        if (!videoDecoder->open(filePath)) {
            handleError(MediaPlayerException::FILE_OPEN_ERROR, "Failed to open video stream");
            return false;
        }

        // Initialize audio decoder
        if (!audioDecoder->open(filePath)) {
            // Video-only file is acceptable
            std::cout << "No audio stream found or audio stream could not be opened" << std::endl;
        }

        // Get duration
        duration = videoDecoder->getDuration();

        // Reset position
        currentPosition = 0.0;

        // Set up initial video frame
        sf::Image image;
        image.create(videoDecoder->getWidth(), videoDecoder->getHeight(), sf::Color::Black);

        frameMutex.lock();
        currentFrame.loadFromImage(image);
        frameMutex.unlock();

        return true;
    } catch (const std::exception& e) {
        handleError(MediaPlayerException::FILE_OPEN_ERROR, "Exception while opening file: " + std::string(e.what()));
        return false;
    }
}

void MediaPlayer::close() {
    // Stop playback if running
    stop();

    // Close decoders
    videoDecoder->close();
    audioDecoder->close();

    // Reset state
    currentPosition = 0.0;
    duration = 0.0;
}

bool MediaPlayer::isOpen() const {
    return videoDecoder->isOpen() || audioDecoder->isOpen();
}

void MediaPlayer::play() {
    if (!isOpen()) {
        handleError(MediaPlayerException::PLAYBACK_ERROR, "No file is open");
        return;
    }

    if (playing && !paused) {
        // Already playing
        return;
    }

    if (paused) {
        // Resume from pause
        paused = false;

        // Resume audio
        sound.play();

        // Call pause callback
        if (playbackPauseCallback) {
            playbackPauseCallback();
        }

        return;
    }

    // Start playback
    playing = true;
    paused = false;

    // Start playback thread if not running
    if (!threadRunning) {
        threadRunning = true;
        playbackThread = std::thread(&MediaPlayer::playbackLoop, this);
    }

    // Call start callback
    if (playbackStartCallback) {
        playbackStartCallback();
    }
}

void MediaPlayer::pause() {
    if (!playing || paused) {
        return;
    }

    // Pause playback
    paused = true;

    // Pause audio
    sound.pause();

    // Call pause callback
    if (playbackPauseCallback) {
        playbackPauseCallback();
    }
}

void MediaPlayer::stop() {
    if (!playing) {
        return;
    }

    // Stop playback
    playing = false;
    paused = false;

    // Stop audio
    sound.stop();

    // Wait for playback thread to finish
    if (threadRunning) {
        threadRunning = false;
        if (playbackThread.joinable()) {
            playbackThread.join();
        }
    }

    // Reset position
    currentPosition = 0.0;

    // Call stop callback
    if (playbackStopCallback) {
        playbackStopCallback();
    }
}

void MediaPlayer::seek(double position) {
    if (!isOpen()) {
        return;
    }

    // Clamp position
    position = std::max<double>(0.0, std::min<double>(position, duration));

    // Seek in decoders
    bool wasPlaying = playing && !paused;

    // Pause playback temporarily
    if (wasPlaying) {
        sound.pause();
    }

    // Seek in decoders
    videoDecoder->seek(position);
    if (audioDecoder->isOpen()) {
        audioDecoder->seek(position);
    }

    // Update position
    currentPosition = position;

    // Decode first frame at new position
    decodeNextVideoFrame();

    // Resume playback if it was playing
    if (wasPlaying) {
        sound.play();
    }

    // Call position change callback
    if (positionChangeCallback) {
        positionChangeCallback(currentPosition);
    }
}

bool MediaPlayer::isPlaying() const {
    return playing && !paused;
}

bool MediaPlayer::isPaused() const {
    return playing && paused;
}

double MediaPlayer::getCurrentPosition() const {
    return currentPosition;
}

double MediaPlayer::getDuration() const {
    return duration;
}

sf::Vector2i MediaPlayer::getVideoSize() const {
    return sf::Vector2i(videoDecoder->getWidth(), videoDecoder->getHeight());
}

void MediaPlayer::setVolume(float volume) {
    // Clamp volume between 0 and 100
    this->volume = std::max<float>(0.0f, std::min<float>(volume, 100.0f));

    // Update sound volume
    sound.setVolume(this->volume);
}

float MediaPlayer::getVolume() const {
    return volume;
}

const sf::Texture& MediaPlayer::getCurrentFrame() const {
    return currentFrame;
}

void MediaPlayer::setPlaybackStartCallback(std::function<void()> callback) {
    playbackStartCallback = std::move(callback);
}

void MediaPlayer::setPlaybackPauseCallback(std::function<void()> callback) {
    playbackPauseCallback = std::move(callback);
}

void MediaPlayer::setPlaybackStopCallback(std::function<void()> callback) {
    playbackStopCallback = std::move(callback);
}

void MediaPlayer::setPlaybackEndCallback(std::function<void()> callback) {
    playbackEndCallback = std::move(callback);
}

void MediaPlayer::setPositionChangeCallback(std::function<void(double)> callback) {
    positionChangeCallback = std::move(callback);
}

void MediaPlayer::setFrameReadyCallback(std::function<void()> callback) {
    frameReadyCallback = std::move(callback);
}

void MediaPlayer::setErrorCallback(std::function<void(const MediaPlayerException&)> callback) {
    errorCallback = std::move(callback);
}

void MediaPlayer::update() {
    // This method should be called regularly from the main thread
    // to update UI and handle events

    // Check for end of playback
    checkEndOfPlayback();

    // Update current position based on audio playback
    updateCurrentPosition();
}

void MediaPlayer::playbackLoop() {
    try {
        // Reset frame clock
        frameClock.restart();

        // Prepare initial audio
        if (audioDecoder->isOpen()) {
            decodeNextAudioChunk();
        }

        // Decode first video frame
        decodeNextVideoFrame();

        // Main playback loop
        while (threadRunning && playing) {
            // Check if paused
            if (paused) {
                // Sleep while paused
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            // Get elapsed time since last frame
            float elapsed = frameClock.getElapsedTime().asSeconds();

            // Check if it's time for a new video frame
            if (elapsed >= frameTime) {
                // Decode next video frame
                if (!decodeNextVideoFrame()) {
                    // End of video
                    break;
                }

                // Reset frame clock
                frameClock.restart();
            }

            // Check if we need more audio
            if (audioDecoder->isOpen() && sound.getStatus() != sf::Sound::Playing) {
                if (!decodeNextAudioChunk()) {
                    // End of audio
                    break;
                }
            }

            // Sleep a bit to avoid using too much CPU
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        // Playback ended
        if (threadRunning) {
            // Only trigger end callback if we weren't stopped manually
            if (playbackEndCallback) {
                playbackEndCallback();
            }
        }

        // Reset state
        playing = false;
        paused = false;
        threadRunning = false;
    } catch (const std::exception& e) {
        handleError(MediaPlayerException::PLAYBACK_ERROR, "Exception in playback thread: " + std::string(e.what()));

        // Reset state
        playing = false;
        paused = false;
        threadRunning = false;
    }
}

bool MediaPlayer::decodeNextVideoFrame() {
    try {
        // Decode next video frame
        if (!videoDecoder->decodeNextFrame()) {
            return false;
        }

        // Get frame data
        const uint8_t* frameData = videoDecoder->getFrameData();
        int width = videoDecoder->getWidth();
        int height = videoDecoder->getHeight();

        // Create image from frame data
        sf::Image image;
        image.create(width, height, frameData);

        // Update current frame
        frameMutex.lock();
        currentFrame.loadFromImage(image);
        frameMutex.unlock();

        // Get frame time (time until next frame)
        frameTime = videoDecoder->getFrameTime();

        // Update current position
        currentPosition = videoDecoder->getCurrentPosition();

        // Call frame ready callback
        if (frameReadyCallback) {
            frameReadyCallback();
        }

        // Call position change callback
        if (positionChangeCallback) {
            positionChangeCallback(currentPosition);
        }

        return true;
    } catch (const std::exception& e) {
        handleError(MediaPlayerException::VIDEO_ERROR, "Error decoding video frame: " + std::string(e.what()));
        return false;
    }
}

bool MediaPlayer::decodeNextAudioChunk() {
    try {
        if (!audioDecoder->isOpen()) {
            return true;
        }

        // Decode audio samples
        audioSamples.clear();
        if (!audioDecoder->decodeAudioSamples(audioSamples, 4096)) {
            return false;
        }

        // Load audio samples into sound buffer
        soundBuffer.loadFromSamples(audioSamples.data(), audioSamples.size(), audioDecoder->getChannels(), audioDecoder->getSampleRate());

        // Set up sound
        sound.setBuffer(soundBuffer);
        sound.setVolume(volume);
        sound.play();

        return true;
    } catch (const std::exception& e) {
        handleError(MediaPlayerException::AUDIO_ERROR, "Error decoding audio: " + std::string(e.what()));
        return false;
    }
}

void MediaPlayer::updateCurrentPosition() {
    if (!playing || paused) {
        return;
    }

    // Update position based on audio playback if available
    if (audioDecoder->isOpen() && sound.getStatus() == sf::Sound::Playing) {
        double audioPosition = audioDecoder->getCurrentPosition();
        if (std::abs(audioPosition - currentPosition) > 0.5) {
            currentPosition = audioPosition;

            // Call position change callback
            if (positionChangeCallback) {
                positionChangeCallback(currentPosition);
            }
        }
    }
}

void MediaPlayer::checkEndOfPlayback() {
    if (!playing) {
        return;
    }

    // Check if we've reached the end of the media
    if (currentPosition >= duration - 0.1) {
        // Stop playback
        stop();

        // Call end callback
        if (playbackEndCallback) {
            playbackEndCallback();
        }
    }
}

void MediaPlayer::handleError(MediaPlayerException::ErrorCode code, const std::string& message) {
    MediaPlayerException error(code, message);

    // Print error to console
    std::cerr << "MediaPlayer error: " << message << std::endl;

    // Call error callback
    if (errorCallback) {
        errorCallback(error);
    }
}

}  // namespace Core
}  // namespace VideoPlayer
