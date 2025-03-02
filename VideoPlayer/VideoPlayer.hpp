#ifndef VIDEOPLAYER_HPP
#define VIDEOPLAYER_HPP

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "SFML/Audio.hpp"
#include "SFML/Graphics.hpp"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"

class VideoPlayer {
 public:
    VideoPlayer();
    ~VideoPlayer();

    bool load(const std::string& filename);

    void play();
    void stop();
    void setVolume(float volume);
    void seek(int frame);
    int getCurrentFrame() const;
    int getTotalFrames() const;
    void draw(sf::RenderWindow& window);

    void addFile(const std::string& filePath);
    void nextFile();
    void previousFile();
    size_t getCurrentFileIndex() const;
    const std::vector<std::string>& getFileList() const;

 private:
    void decodeVideo();
    void decodeAudio();

    AVFormatContext* formatContext;
    AVCodecParameters* videoCodecParameters;
    const AVCodec* videoCodec;
    AVCodecContext* videoCodecContext;
    AVFrame* videoFrame;
    AVPacket* packet;
    SwsContext* swsContext;
    int videoStreamIndex;

    AVCodecParameters* audioCodecParameters;
    const AVCodec* audioCodec;
    AVCodecContext* audioCodecContext;
    int audioStreamIndex;

    sf::Texture texture;
    sf::Sprite sprite;
    sf::Sound sound;
    sf::SoundBuffer soundBuffer;

    float volume;
    std::atomic<bool> isPlaying;
    std::atomic<int> currentFrame;
    std::mutex mutex;

    std::string filePath;
    std::vector<std::string> fileList;
    size_t currentFileIndex;
};

#endif
