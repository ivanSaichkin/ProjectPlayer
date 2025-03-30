#include "../include/Player.hpp"

#include <iostream>
#include <thread>

Player::Player() : timeOffset_(0.0), isRunning_(false) {
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

    timeOffset_ = 0.0;

    startTime_ = std::chrono::steady_clock::now();

    if (videoDecoder_) {
        videoDecoder_->SetStartTime(startTime_);
    }

    if (audioDecoder_) {
        audioDecoder_->SetStartTime(startTime_);
    }

    // Start the demuxing thread that will feed both decoders
    isRunning_ = true;
    isPaused_ = false;
    playbackThread_ = std::thread(&Player::PlaybackLoop, this);
    playbackThread_.detach();
}

void Player::PlaybackLoop() {
    AVPacket* packet = av_packet_alloc();
    if (!packet) {
        std::cerr << "Could not allocate packet" << std::endl;
        isRunning_ = false;
        return;
    }

    // Start decoder threads
    if (videoDecoder_) {
        videoDecoder_->Start();
    }

    if (audioDecoder_) {
        audioDecoder_->Start();
    }

    AVFormatContext* formatContext = mediaFile_.GetFormatContext();
    int videoStreamIndex = mediaFile_.GetVideoStreamIndex();
    int audioStreamIndex = mediaFile_.GetAudioStreamIndex();

    // Read packets and send them to appropriate decoders
    while (isRunning_ && av_read_frame(formatContext, packet) >= 0) {
        // Handle pause state
        while (isPaused_ && isRunning_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        if (!isRunning_) {
            break;
        }

        // Route packet to appropriate decoder
        if (packet->stream_index == videoStreamIndex && videoDecoder_) {
            videoDecoder_->ProcessPacket(packet);
        } else if (packet->stream_index == audioStreamIndex && audioDecoder_) {
            audioDecoder_->ProcessPacket(packet);
        }

        // Unref the packet for reuse
        av_packet_unref(packet);
    }
    // Signal end of stream to decoders
    if (videoDecoder_) {
        videoDecoder_->SignalEndOfStream();
    }

    if (audioDecoder_) {
        audioDecoder_->SignalEndOfStream();
    }

    av_packet_free(&packet);
    isRunning_ = false;

    std::cout << "Playback finished" << std::endl;
}

void Player::TogglePause() {
    isPaused_ = !isPaused_;

    if (videoDecoder_) {
        videoDecoder_->SetPaused(isPaused_);
    }

    if (audioDecoder_) {
        audioDecoder_->SetPaused(isPaused_);
    }

    // Update time offset when pausing/resuming
    timeOffset_ = GetCurrentTime();
    startTime_ = std::chrono::steady_clock::now() -
                 std::chrono::steady_clock::duration(
                     std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(timeOffset_)));
}

void Player::Stop() {
    isRunning_ = false;

    if (videoDecoder_) {
        videoDecoder_->Stop();
    }

    if (audioDecoder_) {
        audioDecoder_->Stop();
    }

    timeOffset_ = 0.0;

    // Wait for playback thread to finish
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void Player::Draw(Window& window) {
    if (videoDecoder_) {
        sf::Sprite& sprite = videoDecoder_->GetSprite();

        // Масштабируем спрайт под размер окна с сохранением пропорций
        window.ScaleSprite(sprite, true);

        // Отрисовываем видеокадр
        window.GetRenderWindow().draw(sprite);
    }
}

void Player::Seek(int seconds) {
    if (!mediaFile_.GetFormatContext()) {
        return;
    }

    isPaused_ = true;

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
        isPaused_ = false;
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
    startTime_ = std::chrono::steady_clock::now() -
                 std::chrono::steady_clock::duration(
                     std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(timeOffset_)));

    if (videoDecoder_) {
        videoDecoder_->SetStartTime(startTime_);
    }

    if (audioDecoder_) {
        audioDecoder_->SetStartTime(startTime_);
    }

    isPaused_ = false;
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

float Player::GetVolume() const {
    if (audioDecoder_) {
        return audioDecoder_->GetVolume();
    }
    return 0.0f;
}

sf::Vector2i Player::GetVideoSize() const {
    if (videoDecoder_) {
        return videoDecoder_->GetSize();
    }
    return sf::Vector2i(0, 0);
}
