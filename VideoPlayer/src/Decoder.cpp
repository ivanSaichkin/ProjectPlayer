#include "../include/Decoder.hpp"

void Decoder::Stop() {
    isRunning_ = false;
    packetCondition_.notify_all();
}

void Decoder::SetPaused(bool paused) {
    isPaused_ = paused;
}

void Decoder::SetStartTime(std::chrono::steady_clock::time_point startTime) {
    std::lock_guard<std::mutex> lock(mutex_);
    startTime_ = startTime;
}
