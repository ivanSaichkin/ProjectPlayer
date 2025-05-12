#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <vector>

// Forward declarations for FFmpeg structures to avoid including FFmpeg headers in our header
struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwsContext;

namespace VideoPlayer {
namespace Core {

class VideoDecoder {
 public:
    // Constructor and destructor
    VideoDecoder();
    ~VideoDecoder();

    // File operations
    bool open(const std::string& filePath);
    void close();
    bool isOpen() const;

    // Decoding
    bool decodeNextFrame();
    bool seek(double position);

    // Frame information
    const uint8_t* getFrameData() const;
    int getWidth() const;
    int getHeight() const;
    double getFrameTime() const;
    double getCurrentPosition() const;
    double getDuration() const;

    // Stream information
    int getStreamIndex() const;
    double getFrameRate() const;
    std::string getCodecName() const;

 private:
    // FFmpeg contexts
    AVFormatContext* formatContext;
    AVCodecContext* codecContext;
    SwsContext* swsContext;

    // FFmpeg frames and packets
    AVFrame* frame;
    AVFrame* rgbFrame;
    AVPacket* packet;

    // Stream information
    int videoStreamIndex;
    double frameRate;
    double timeBase;
    double duration;
    double currentPosition;

    // Frame data
    std::vector<uint8_t> frameBuffer;
    int width;
    int height;

    // Mutex for thread safety
    std::mutex mutex;

    // Internal methods
    bool initializeDecoder(const std::string& filePath);
    void cleanupDecoder();
    bool readFrame();
    bool decodePacket();
    bool convertFrame();
    void updatePosition();
};

}  // namespace Core
}  // namespace VideoPlayer
