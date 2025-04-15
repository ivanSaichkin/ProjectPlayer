#include "MediaPlayer.hpp"

#include <iostream>

// CustomAudioStream implementation
MediaPlayer::CustomAudioStream::CustomAudioStream(AudioDecoder& decoder) : audioDecoder(decoder) {
    // Initialize audio stream
    initialize(decoder.getChannelCount(), decoder.getSampleRate());
}

void MediaPlayer::CustomAudioStream::start() {
    play();
}

void MediaPlayer::CustomAudioStream::stop() {
    sf::SoundStream::stop();
}

bool MediaPlayer::CustomAudioStream::onGetData(Chunk& data) {
    // Get next audio packet
    AudioPacket packet;
    if (!audioDecoder.getNextPacket(packet)) {
        return false;
    }

    // Set buffer data
    buffer = std::move(packet.samples);
    data.samples = buffer.data();
    data.sampleCount = buffer.size();

    return true;
}

void MediaPlayer::CustomAudioStream::onSeek(sf::Time timeOffset) {
    // Not implemented, seeking is handled by MediaPlayer
}

// MediaPlayer implementation
MediaPlayer::MediaPlayer() : playing(false), volume(1.0f), currentPosition(0.0), newFrameAvailable(false) {
    // Set error callback
    ErrorHandler::getInstance().setErrorCallback([this](const MediaPlayerException& e) {
        if (e.getCode() == MediaPlayerException::FILE_NOT_FOUND || e.getCode() == MediaPlayerException::DECODER_ERROR) {
            close();
        }
    });
}

MediaPlayer::~MediaPlayer() {
    close();
}

bool MediaPlayer::open(const std::string& filename) {
    // Close any previously opened file
    close();

    // Open the media file
    if (!videoDecoder.open(filename)) {
        return false;
    }

    // Initialize video decoder
    if (!videoDecoder.initialize()) {
        videoDecoder.close();
        return false;
    }

    // Initialize audio decoder if available
    bool hasAudio = audioDecoder.open(filename) && audioDecoder.initialize();

    // Start video decoding
    videoDecoder.start();

    // Start audio decoding if available
    if (hasAudio) {
        audioDecoder.start();

        // Create audio stream
        audioStream = std::make_unique<CustomAudioStream>(audioDecoder);
    }

    // Reset position and state
    currentPosition = 0.0;
    playing = false;
    newFrameAvailable = false;

    return true;
}

void MediaPlayer::close() {
    // Stop playback
    if (playing) {
        pause();
    }

    // Stop and close decoders
    videoDecoder.stop();
    videoDecoder.close();

    audioDecoder.stop();
    audioDecoder.close();

    // Reset audio stream
    audioStream.reset();

    // Reset state
    currentPosition = 0.0;
    playing = false;
    newFrameAvailable = false;

    // Call stop callback
    if (playbackStopCallback) {
        playbackStopCallback();
    }
}

void MediaPlayer::play() {
    if (playing) {
        return;
    }

    if (!videoDecoder.isOpen()) {
        ErrorHandler::getInstance().handleError(MediaPlayerException::DECODER_ERROR, "Cannot play: no file is open");
        return;
    }

    // Start decoders
    videoDecoder.setPaused(false);
    audioDecoder.setPaused(false);

    // Start audio playback if available
    if (audioStream) {
        audioStream->start();
    }

    // Update state
    playing = true;
    positionClock.restart();

    // Call start callback
    if (playbackStartCallback) {
        playbackStartCallback();
    }
}

void MediaPlayer::pause() {
    if (!playing) {
        return;
    }

    // Pause decoders
    videoDecoder.setPaused(true);
    audioDecoder.setPaused(true);

    // Pause audio playback if available
    if (audioStream) {
        audioStream->stop();
    }

    // Update state
    playing = false;

    // Update position
    updatePosition();

    // Call pause callback
    if (playbackPauseCallback) {
        playbackPauseCallback();
    }
}

void MediaPlayer::togglePlayPause() {
    if (playing) {
        pause();
    } else {
        play();
    }
}

void MediaPlayer::seek(double seconds) {
    if (!videoDecoder.isOpen()) {
        ErrorHandler::getInstance().handleError(MediaPlayerException::DECODER_ERROR, "Cannot seek: no file is open");
        return;
    }

    // Clamp position to valid range
    double duration = getDuration();
    if (seconds < 0) {
        seconds = 0;
    } else if (seconds > duration) {
        seconds = duration;
    }

    // Pause playback temporarily
    bool wasPlaying = playing;
    if (playing) {
        pause();
    }

    // Seek in decoders
    videoDecoder.seek(seconds);
    audioDecoder.seek(seconds);

    // Update position
    currentPosition = seconds;
    positionClock.restart();

    // Notify position change
    notifyPositionChange();

    // Resume playback if it was playing
    if (wasPlaying) {
        play();
    }
}

void MediaPlayer::setVolume(float volume) {
    // Clamp volume to valid range
    if (volume < 0.0f) {
        volume = 0.0f;
    } else if (volume > 1.0f) {
        volume = 1.0f;
    }

    this->volume = volume;

    // Set volume on audio stream if available
    if (audioStream) {
        audioStream->setVolume(volume * 100.0f);
    }
}

float MediaPlayer::getVolume() const {
    return volume;
}

bool MediaPlayer::isPlaying() const {
    return playing;
}

double MediaPlayer::getDuration() const {
    return videoDecoder.getDuration();
}

double MediaPlayer::getCurrentPosition() const {
    return currentPosition;
}

sf::Vector2u MediaPlayer::getVideoSize() const {
    return videoDecoder.getSize();
}

double MediaPlayer::getFrameRate() const {
    return videoDecoder.getFrameRate();
}

unsigned int MediaPlayer::getAudioSampleRate() const {
    return audioDecoder.getSampleRate();
}

unsigned int MediaPlayer::getAudioChannelCount() const {
    return audioDecoder.getChannelCount();
}

bool MediaPlayer::getCurrentFrame(sf::Texture& texture) {
    std::lock_guard<std::mutex> lock(frameMutex);

    if (!newFrameAvailable) {
        return false;
    }

    texture = currentFrame.texture;
    newFrameAvailable = false;

    return true;
}

void MediaPlayer::update() {
    // Update position if playing
    if (playing) {
        updatePosition();
    }

    // Get next video frame if available
    VideoFrame frame;
    if (videoDecoder.getNextFrame(frame)) {
        std::lock_guard<std::mutex> lock(frameMutex);
        currentFrame = frame;
        newFrameAvailable = true;

        // Call frame ready callback
        if (frameReadyCallback) {
            frameReadyCallback();
        }
    }
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

void MediaPlayer::setPositionChangeCallback(std::function<void(double)> callback) {
    positionChangeCallback = std::move(callback);
}

void MediaPlayer::setFrameReadyCallback(std::function<void()> callback) {
    frameReadyCallback = std::move(callback);
}

void MediaPlayer::setErrorCallback(std::function<void(const MediaPlayerException&)> callback) {
    ErrorHandler::getInstance().setErrorCallback(std::move(callback));
}

void MediaPlayer::updatePosition() {
    if (playing) {
        // Update position based on elapsed time
        double current = currentPosition.load();
        double newPosition = current + positionClock.restart().asSeconds();
        currentPosition.store(newPosition);
    }

    // Notify position change
    notifyPositionChange();
}

void MediaPlayer::notifyPositionChange() {
    if (positionChangeCallback) {
        positionChangeCallback(currentPosition);
    }
}
