#ifndef DECODER_HPP
#define DECODER_HPP

#include <atomic>
#include <functional>
#include <string>

#include "AudioManager.hpp"
#include "FrameManager.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

class Decoder {
 public:
    Decoder();
    ~Decoder();

    bool Open(const std::string& filename);
    void Close();
    bool IsOpen() const;

    // Декодирование всего файла заранее
    bool DecodeAll(std::function<void(float)> progressCallback = nullptr);

    // Доступ к декодированным данным
    FrameManager& GetFrameManager();
    AudioManager& GetAudioManager();

    // Информация о файле
    double GetDuration() const;
    int GetWidth() const;
    int GetHeight() const;
    int GetFrameRate() const;
    int GetAudioSampleRate() const;
    int GetAudioChannels() const;

 private:
    // Методы для декодирования
    bool DecodeVideoFrame(AVFrame* frame, double timestamp);
    bool DecodeAudioFrame(AVFrame* frame, double timestamp);

    // Инициализация кодеков
    bool InitVideoCodec();
    bool InitAudioCodec();

    // Переменные для FFmpeg
    AVFormatContext* formatContext_;
    AVCodecContext* videoCodecContext_;
    AVCodecContext* audioCodecContext_;
    SwsContext* swsContext_;
    SwrContext* swrContext_;
    int videoStreamIndex_;
    int audioStreamIndex_;

    // Менеджеры данных
    FrameManager frameManager_;
    AudioManager audioManager_;

    // Флаги состояния
    std::atomic<bool> isOpen_;
    std::atomic<bool> isDecoding_;
};

#endif
