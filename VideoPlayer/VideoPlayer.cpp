#include "VideoPlayer.hpp"

#include "tinyfiledialogs/tinyfiledialogs.h"

VideoPlayer::VideoPlayer()
    : formatContext(nullptr),
      videoCodecParameters(nullptr),
      videoCodec(nullptr),
      videoCodecContext(nullptr),
      videoFrame(nullptr),
      packet(nullptr),
      swsContext(nullptr),
      videoStreamIndex(-1),
      audioCodecParameters(nullptr),
      audioCodec(nullptr),
      audioCodecContext(nullptr),
      audioStreamIndex(-1),
      volume(100),
      isPlaying(false),
      currentFrame(0),
      filePath(""),
      currentFileIndex(0) {
}

VideoPlayer::~VideoPlayer() {
    stop();

    if (videoFrame) {
        av_frame_free(&videoFrame);
    }

    if (packet) {
        av_packet_free(&packet);
    }

    if (videoCodecContext) {
        avcodec_free_context(&videoCodecContext);
    }

    if (audioCodecContext) {
        avcodec_free_context(&audioCodecContext);
    }

    if (formatContext) {
        avformat_close_input(&formatContext);
    }

    if (swsContext) {
        sws_freeContext(swsContext);
    }
}

bool VideoPlayer::load(const std::string& filename) {
    this->filePath = filePath;

    if (avformat_open_input(&formatContext, filePath.c_str(), nullptr, nullptr) != 0) {
        throw std::runtime_error("Couldn't open");
    }

    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
        throw std::runtime_error("Couldn't find stream info");
    }

    videoStreamIndex = -1;
    audioStreamIndex = -1;

    for (size_t i = 0; i < formatContext->nb_streams; ++i) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = 1;
        } else if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStreamIndex = i;
        }
    }

    if (videoStreamIndex == -1) {
        throw std::runtime_error("Couldn't find a video stream");
    }

    videoCodecParameters = formatContext->streams[videoStreamIndex]->codecpar;
    videoCodec = avcodec_find_decoder(videoCodecParameters->codec_id);
    if (!videoCodec) {
        throw std::runtime_error("Unsupported video codec");
    }

    videoCodecContext = avcodec_alloc_context3(videoCodec);
    avcodec_parameters_to_context(videoCodecContext, videoCodecParameters);

    if (avcodec_open2(videoCodecContext, videoCodec, nullptr) < 0) {
        throw std::runtime_error("Failed to open video codec");
    }

    videoFrame = av_frame_alloc();
    packet = av_packet_alloc();

    swsContext = sws_getContext(videoCodecContext->width, videoCodecContext->height, videoCodecContext->pix_fmt, videoCodecContext->width,
                                videoCodecContext->height, AV_PIX_FMT_RGB32, SWS_BILINEAR, nullptr, nullptr, nullptr);
    texture.create(videoCodecContext->width, videoCodecContext->height);
    sprite.setTexture(texture);

    if (audioStreamIndex != -1) {
        audioCodecParameters = formatContext->streams[audioStreamIndex]->codecpar;
        audioCodec = avcodec_find_decoder(audioCodecParameters->codec_id);
        if (!audioCodec) {
            throw std::runtime_error("Unsupported audio codec");
        }

        audioCodecContext = avcodec_alloc_context3(audioCodec);
        avcodec_parameters_to_context(audioCodecContext, audioCodecParameters);

        if (avcodec_open2(audioCodecContext, audioCodec, nullptr) < 0) {
            throw std::runtime_error("Failed to open audio codec");
        }

        soundBuffer.loadFromSamples(nullptr, 0, audioCodecContext->sample_rate, audioCodecContext->channels);
        sound.setBuffer(soundBuffer);
        sound.setVolume(volume);
    }

    return true;
}

void VideoPlayer::play() {
    isPlaying = true;
    std::thread([this]() { decodeVideo(); }).detach();

    if (audioStreamIndex != -1) {
        std::thread([this]() { decodeAudio(); }).detach();
    }
}

void VideoPlayer::stop() {
    isPlaying = false;
    sound.stop();
}

void VideoPlayer::setVolume(float volume) {
    this->volume = volume;
    sound.setVolume(volume);
}

void VideoPlayer::seek(int frame) {
    std::unique_lock<std::mutex> lock(mutex);
    av_seek_frame(formatContext, videoStreamIndex, frame, AVSEEK_FLAG_BACKWARD);
    currentFrame = frame;
}

int VideoPlayer::getCurrentFrame() const {
    return currentFrame;
}

int VideoPlayer::getTotalFrames() const {
    return formatContext->streams[videoStreamIndex]->nb_frames;
}

void VideoPlayer::draw(sf::RenderWindow& window) {
    window.draw(sprite);
}

void VideoPlayer::addFile(const std::string& filePath) {
    fileList.push_back(filePath);
}

void VideoPlayer::nextFile() {
    if (fileList.empty()) {
        return;
    }

    currentFileIndex = (currentFileIndex + 1) % fileList.size();
    load(fileList[currentFileIndex]);
}

void VideoPlayer::previousFile() {
    if (fileList.empty()) {
        return;
    }

    currentFileIndex = (currentFileIndex - 1 + fileList.size()) % fileList.size();
    load(fileList[currentFileIndex]);
}

size_t VideoPlayer::getCurrentFileIndex() const {
    return currentFileIndex;
}

const std::vector<std::string>& VideoPlayer::getFileList() const {
    return fileList;
}

void VideoPlayer::decodeVideo() {
    while (isPlaying) {
        if (av_read_frame(formatContext, packet) >= 0) {
            if (packet->stream_index == videoStreamIndex) {
                if (avcodec_send_packet(videoCodecContext, packet) == 0) {
                    while (avcodec_receive_frame(videoCodecContext, videoFrame) == 0) {
                        std::unique_lock<std::mutex> lock(mutex);

                        uint8_t* pixels = new uint8_t[videoCodecContext->width * videoCodecContext->height * 4];
                        uint8_t* dest[4] = {pixels, nullptr, nullptr, nullptr};
                        int destLinesize[4] = {videoCodecContext->width * 4, 0, 0, 0};

                        sws_scale(swsContext, videoFrame->data, videoFrame->linesize, 0, videoCodecContext->height, dest, destLinesize);

                        texture.update(pixels);
                        delete[] pixels;

                        currentFrame++;
                        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / videoCodecContext->framerate.num));
                    }
                }
            }
            av_packet_unref(packet);
        } else {
            isPlaying = false;
        }
    }
}

void VideoPlayer::decodeAudio() {
    while (isPlaying) {
        if (av_read_frame(formatContext, packet) >= 0) {
            if (packet->stream_index == audioStreamIndex) {
                if (avcodec_send_packet(audioCodecContext, packet) == 0) {
                    AVFrame* audioFrame = av_frame_alloc();

                    while (avcodec_receive_frame(audioCodecContext, audioFrame) == 0) {
                        std::vector<int16_t> samples(audioFrame->nb_samples * audioFrame->channels);

                        for (int i = 0; i < audioFrame->nb_samples; ++i) {
                            for (int j = 0; j < audioFrame->channels; ++j) {
                                samples[i * audioFrame->channels + j] = ((uint16_t*)audioFrame->data[j])[i];
                            }
                        }

                        soundBuffer.loadFromSamples(samples.data(), samples.size(), audioFrame->channels, audioFrame->sample_rate);
                        sound.play();
                    }
                    av_frame_free(&audioFrame);
                }
            }
            av_packet_unref(packet);
        } else {
            isPlaying = false;
        }
    }
}
