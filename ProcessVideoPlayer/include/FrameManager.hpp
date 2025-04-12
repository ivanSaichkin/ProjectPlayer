#ifndef FRAMEMANAGER_HPP
#define FRAMEMANAGER_HPP

#include <memory>
#include <mutex>
#include <vector>

#include "Frame.hpp"

class FrameManager {
 public:
    FrameManager();
    ~FrameManager();

    void AddFrame(std::unique_ptr<Frame> frame);
    const Frame* GetFrameAtTime(double timestamp) const;
    const Frame* GetFrameAtIndex(size_t index) const;
    size_t GetFrameCount() const;
    double GetDuration() const;
    void Clear();

 private:
    std::vector<std::unique_ptr<Frame>> frames_;
    mutable std::mutex mutex_;
};

#endif
