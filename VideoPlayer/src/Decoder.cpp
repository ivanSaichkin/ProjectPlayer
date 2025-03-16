#include "../include/Decoder.hpp"

void Decoder::Stop(){
    isRunning_ = false;
}

void Decoder::TogglePause(){
    isPaused_ = !isPaused_;
}

void Decoder::SetStartTime(std::chrono::steady_clock::time_point startTime){
    startTime_ = startTime;
}
