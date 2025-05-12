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
struct SwrContext;

namespace VideoPlayer {
namespace Core {

class AudioDecoder {
 public:
    // Constructor and destructor
    AudioDecoder();
    ~AudioDecoder();

    // File operations
    bool open(const std::string& filePath);
    void close();
    bool isOpen() const;

    // Decoding
    bool decodeAudioSamples(std::vector<short>& samples, int numSamples);
    bool seek(double position);

    // Audio information
    int getSampleRate() const;
    int getChannels() const;
    double getCurrentPosition() const;
    double getDuration() const;

    // Stream information
    int getStreamIndex() const;
    std::string getCodecName() const;

 private:
    // FFmpeg contexts
    AVFormatContext* formatContext;
    AVCodecContext* codecContext;
    SwrContext* swrContext;

    // FFmpeg frames and packets
    AVFrame* frame;
    AVPacket* packet;

    // Stream information
    int audioStreamIndex;
    double timeBase;
    double duration;
    double currentPosition;

    // Audio parameters
    int sampleRate;
    int channels;

    // Buffering
    std::vector<short> audioBuffer;
    size_t bufferPosition;

    // Mutex for thread safety
    std::mutex mutex;

    // Internal methods
    bool initializeDecoder(const std::string& filePath);
    void cleanupDecoder();
    bool readFrame();
    bool decodePacket();
    bool convertFrame(std::vector<short>& outSamples);
    void updatePosition();
};

}  // namespace Core
}  // namespace VideoPlayer
