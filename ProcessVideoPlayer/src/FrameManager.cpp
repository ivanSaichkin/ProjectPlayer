#include "../include/FrameManager.hpp"

#include <algorithm>

FrameManager::FrameManager() {
}

FrameManager::~FrameManager() {
    Clear();
}

void FrameManager::AddFrame(std::unique_ptr<Frame> frame) {
    std::lock_guard<std::mutex> lock(mutex_);
    frames_.push_back(std::move(frame));
}

const Frame* FrameManager::GetFrameAtTime(double timestamp) const {
    std::lock_guard<std::mutex> lock(mutex_);

    if (frames_.empty()) {
        return nullptr;
    }

    // Если время до первого кадра, возвращаем первый кадр
    if (timestamp < frames_[0]->GetTimestamp()) {
        return frames_[0].get();
    }

    // Если время после последнего кадра, возвращаем последний кадр
    if (timestamp >= frames_.back()->GetTimestamp()) {
        return frames_.back().get();
    }

    // Бинарный поиск для нахождения кадра
    auto it = std::lower_bound(frames_.begin(), frames_.end(), timestamp,
                               [](const std::unique_ptr<Frame>& frame, double time) { return frame->GetTimestamp() < time; });

    // Возвращаем кадр, ближайший к запрошенному времени
    if (it != frames_.begin() && timestamp - (*(it - 1))->GetTimestamp() < (*it)->GetTimestamp() - timestamp) {
        return (*(it - 1)).get();
    }

    return (*it).get();
}

const Frame* FrameManager::GetFrameAtIndex(size_t index) const {
    std::lock_guard<std::mutex> lock(mutex_);

    if (index >= frames_.size()) {
        return nullptr;
    }

    return frames_[index].get();
}

size_t FrameManager::GetFrameCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return frames_.size();
}

double FrameManager::GetDuration() const {
    std::lock_guard<std::mutex> lock(mutex_);

    if (frames_.empty()) {
        return 0.0;
    }

    return frames_.back()->GetTimestamp();
}

void FrameManager::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    frames_.clear();
}
