#include "../include/Decoder.hpp"

#include <chrono>
#include <iostream>
#include <thread>

Decoder::Decoder()
    : formatContext_(nullptr),
      videoCodecContext_(nullptr),
      audioCodecContext_(nullptr),
      swsContext_(nullptr),
      swrContext_(nullptr),
      videoStreamIndex_(-1),
      audioStreamIndex_(-1),
      isOpen_(false),
      isDecoding_(false) {
}

Decoder::~Decoder() {
    Close();
}

bool Decoder::Open(const std::string& filename) {
    // Закрываем предыдущий файл, если он был открыт
    Close();

    // Открываем файл
    formatContext_ = avformat_alloc_context();
    if (!formatContext_) {
        std::cerr << "Failed to allocate format context" << std::endl;
        return false;
    }

    if (avformat_open_input(&formatContext_, filename.c_str(), nullptr, nullptr) != 0) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        Close();
        return false;
    }

    if (avformat_find_stream_info(formatContext_, nullptr) < 0) {
        std::cerr << "Failed to find stream info" << std::endl;
        Close();
        return false;
    }

    // Находим видео и аудио потоки
    videoStreamIndex_ = -1;
    audioStreamIndex_ = -1;

    for (unsigned int i = 0; i < formatContext_->nb_streams; i++) {
        if (formatContext_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && videoStreamIndex_ < 0) {
            videoStreamIndex_ = i;
        } else if (formatContext_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audioStreamIndex_ < 0) {
            audioStreamIndex_ = i;
        }
    }

    // Инициализируем кодеки
    bool videoInitialized = InitVideoCodec();
    bool audioInitialized = InitAudioCodec();

    if (!videoInitialized && !audioInitialized) {
        std::cerr << "Failed to initialize any codec" << std::endl;
        Close();
        return false;
    }

    isOpen_ = true;
    return true;
}

void Decoder::Close() {
    isDecoding_ = false;
    isOpen_ = false;

    frameManager_.Clear();
    audioManager_.Clear();

    if (swrContext_) {
        swr_free(&swrContext_);
        swrContext_ = nullptr;
    }

    if (swsContext_) {
        sws_freeContext(swsContext_);
        swsContext_ = nullptr;
    }

    if (videoCodecContext_) {
        avcodec_free_context(&videoCodecContext_);
        videoCodecContext_ = nullptr;
    }

    if (audioCodecContext_) {
        avcodec_free_context(&audioCodecContext_);
        audioCodecContext_ = nullptr;
    }

    if (formatContext_) {
        avformat_close_input(&formatContext_);
        formatContext_ = nullptr;
    }

    videoStreamIndex_ = -1;
    audioStreamIndex_ = -1;
}

bool Decoder::IsOpen() const {
    return isOpen_;
}

bool Decoder::DecodeAll(std::function<void(float)> progressCallback) {
    if (!isOpen_ || isDecoding_) {
        return false;
    }

    isDecoding_ = true;

    // Очищаем предыдущие данные
    frameManager_.Clear();
    audioManager_.Clear();

    // Перематываем в начало файла
    if (av_seek_frame(formatContext_, -1, 0, AVSEEK_FLAG_BACKWARD) < 0) {
        std::cerr << "Failed to seek to beginning of file" << std::endl;
        isDecoding_ = false;
        return false;
    }

    // Выделяем память для пакета и кадра
    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();

    if (!packet || !frame) {
        std::cerr << "Failed to allocate packet or frame" << std::endl;
        if (packet)
            av_packet_free(&packet);
        if (frame)
            av_frame_free(&frame);
        isDecoding_ = false;
        return false;
    }

    // Получаем длительность файла для отслеживания прогресса
    double duration = GetDuration();
    double currentTime = 0.0;

    // Читаем и декодируем все пакеты
    while (isDecoding_ && av_read_frame(formatContext_, packet) >= 0) {
        bool packetProcessed = false;

        // Обрабатываем видеопакеты
        if (packet->stream_index == videoStreamIndex_ && videoCodecContext_) {
            int response = avcodec_send_packet(videoCodecContext_, packet);
            if (response >= 0) {
                while (response >= 0) {
                    response = avcodec_receive_frame(videoCodecContext_, frame);
                    if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
                        break;
                    } else if (response < 0) {
                        std::cerr << "Error during video decoding" << std::endl;
                        break;
                    }

                    // Вычисляем временную метку кадра
                    double timestamp = frame->pts * av_q2d(formatContext_->streams[videoStreamIndex_]->time_base);

                    // Декодируем и сохраняем кадр
                    DecodeVideoFrame(frame, timestamp);

                    // Обновляем текущее время для отслеживания прогресса
                    currentTime = timestamp;

                    // Вызываем callback для обновления прогресса
                    if (progressCallback && duration > 0) {
                        progressCallback(static_cast<float>(currentTime / duration));
                    }
                }
                packetProcessed = true;
            }
        }

        // Обрабатываем аудиопакеты
        if (packet->stream_index == audioStreamIndex_ && audioCodecContext_) {
            int response = avcodec_send_packet(audioCodecContext_, packet);
            if (response >= 0) {
                while (response >= 0) {
                    response = avcodec_receive_frame(audioCodecContext_, frame);
                    if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
                        break;
                    } else if (response < 0) {
                        std::cerr << "Error during audio decoding" << std::endl;
                        break;
                    }

                    // Вычисляем временную метку аудиосемпла
                    double timestamp = frame->pts * av_q2d(formatContext_->streams[audioStreamIndex_]->time_base);

                    // Декодируем и сохраняем аудио
                    DecodeAudioFrame(frame, timestamp);
                }
                packetProcessed = true;
            }
        }

        av_packet_unref(packet);

        // Пауза для предотвращения блокировки UI
        if (!packetProcessed) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    // Очищаем кодеки от оставшихся кадров
    if (videoCodecContext_) {
        avcodec_send_packet(videoCodecContext_, nullptr);
        while (true) {
            int response = avcodec_receive_frame(videoCodecContext_, frame);
            if (response != 0)
                break;

            double timestamp = frame->pts * av_q2d(formatContext_->streams[videoStreamIndex_]->time_base);
            DecodeVideoFrame(frame, timestamp);
        }
    }

    if (audioCodecContext_) {
        avcodec_send_packet(audioCodecContext_, nullptr);
        while (true) {
            int response = avcodec_receive_frame(audioCodecContext_, frame);
            if (response != 0)
                break;

            double timestamp = frame->pts * av_q2d(formatContext_->streams[audioStreamIndex_]->time_base);
            DecodeAudioFrame(frame, timestamp);
        }
    }

    // Освобождаем ресурсы
    av_frame_free(&frame);
    av_packet_free(&packet);

    // Вызываем callback для обновления прогресса (100%)
    if (progressCallback) {
        progressCallback(1.0f);
    }

    isDecoding_ = false;
    return true;
}

FrameManager& Decoder::GetFrameManager() {
    return frameManager_;
}

AudioManager& Decoder::GetAudioManager() {
    return audioManager_;
}

double Decoder::GetDuration() const {
    if (!formatContext_)
        return 0.0;

    double duration = static_cast<double>(formatContext_->duration) / AV_TIME_BASE;
    return duration > 0 ? duration : 0.0;
}

int Decoder::GetWidth() const {
    return videoCodecContext_ ? videoCodecContext_->width : 0;
}

int Decoder::GetHeight() const {
    return videoCodecContext_ ? videoCodecContext_->height : 0;
}

int Decoder::GetFrameRate() const {
    if (!formatContext_ || videoStreamIndex_ < 0)
        return 0;

    AVStream* stream = formatContext_->streams[videoStreamIndex_];
    return static_cast<int>(av_q2d(stream->avg_frame_rate));
}

int Decoder::GetAudioSampleRate() const {
    return audioCodecContext_ ? audioCodecContext_->sample_rate : 0;
}

int Decoder::GetAudioChannels() const {
    return audioCodecContext_ ? audioCodecContext_->channels : 0;
}

bool Decoder::DecodeVideoFrame(AVFrame* frame, double timestamp) {
    if (!videoCodecContext_ || !frame) {
        return false;
    }

    // Создаем контекст для преобразования, если он еще не создан
    if (!swsContext_) {
        swsContext_ = sws_getContext(videoCodecContext_->width, videoCodecContext_->height, videoCodecContext_->pix_fmt, videoCodecContext_->width,
                                     videoCodecContext_->height, AV_PIX_FMT_RGBA, SWS_BILINEAR, nullptr, nullptr, nullptr);

        if (!swsContext_) {
            std::cerr << "Failed to create scaling context" << std::endl;
            return false;
        }
    }

    // Выделяем память для RGBA данных
    int bufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGBA, videoCodecContext_->width, videoCodecContext_->height, 1);
    std::vector<uint8_t> buffer(bufferSize);

    // Настраиваем указатели для преобразования
    uint8_t* dest[4] = {buffer.data(), nullptr, nullptr, nullptr};
    int destLinesize[4] = {videoCodecContext_->width * 4, 0, 0, 0};

    // Выполняем преобразование
    sws_scale(swsContext_, frame->data, frame->linesize, 0, videoCodecContext_->height, dest, destLinesize);

    // Создаем текстуру SFML
    sf::Texture texture;
    if (!texture.create(videoCodecContext_->width, videoCodecContext_->height)) {
        std::cerr << "Failed to create texture" << std::endl;
        return false;
    }

    // Обновляем текстуру данными кадра
    texture.update(buffer.data());

    // Создаем объект кадра и добавляем его в менеджер
    auto newFrame = std::make_unique<Frame>(timestamp, texture);
    frameManager_.AddFrame(std::move(newFrame));

    return true;
}

bool Decoder::DecodeAudioFrame(AVFrame* frame, double timestamp) {
    if (!audioCodecContext_ || !frame) {
        return false;
    }

    // Создаем контекст для преобразования, если он еще не создан
    if (!swrContext_) {
        int64_t channelLayout = frame->channel_layout;
        if (channelLayout == 0) {
            channelLayout = av_get_default_channel_layout(audioCodecContext_->channels);
        }

        swrContext_ = swr_alloc_set_opts(nullptr,
                                         AV_CH_LAYOUT_STEREO,  // Выходной формат - стерео
                                         AV_SAMPLE_FMT_S16,    // Выходной формат - 16-бит signed int
                                         audioCodecContext_->sample_rate, channelLayout, static_cast<AVSampleFormat>(frame->format),
                                         frame->sample_rate, 0, nullptr);

        if (!swrContext_ || swr_init(swrContext_) < 0) {
            std::cerr << "Failed to create resampling context" << std::endl;
            return false;
        }
    }

    // Вычисляем размер выходного буфера
    int outSamples = static_cast<int>(av_rescale_rnd(swr_get_delay(swrContext_, frame->sample_rate) + frame->nb_samples,
                                                     audioCodecContext_->sample_rate, frame->sample_rate, AV_ROUND_UP));

    // Выделяем буфер для преобразованных семплов
    std::vector<int16_t> outBuffer(outSamples * 2);  // 2 канала (стерео)
    uint8_t* outData[1] = {reinterpret_cast<uint8_t*>(outBuffer.data())};

    // Выполняем преобразование
    int samplesOut = swr_convert(swrContext_, outData, outSamples, const_cast<const uint8_t**>(frame->data), frame->nb_samples);

    if (samplesOut <= 0) {
        std::cerr << "Error resampling audio" << std::endl;
        return false;
    }

    // Корректируем размер буфера
    outBuffer.resize(samplesOut * 2);

    // Создаем аудиочанк и добавляем его в менеджер
    auto newChunk = std::make_unique<AudioChunk>(timestamp, outBuffer, audioCodecContext_->sample_rate, 2);

    audioManager_.AddChunk(std::move(newChunk));

    return true;
}

bool Decoder::InitVideoCodec() {
    if (videoStreamIndex_ < 0) {
        return false;
    }

    AVStream* stream = formatContext_->streams[videoStreamIndex_];
    const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);

    if (!codec) {
        std::cerr << "Unsupported video codec" << std::endl;
        return false;
    }

    videoCodecContext_ = avcodec_alloc_context3(codec);
    if (!videoCodecContext_) {
        std::cerr << "Failed to allocate video codec context" << std::endl;
        return false;
    }

    if (avcodec_parameters_to_context(videoCodecContext_, stream->codecpar) < 0) {
        std::cerr << "Failed to copy video codec parameters" << std::endl;
        avcodec_free_context(&videoCodecContext_);
        videoCodecContext_ = nullptr;
        return false;
    }

    if (avcodec_open2(videoCodecContext_, codec, nullptr) < 0) {
        std::cerr << "Failed to open video codec" << std::endl;
        avcodec_free_context(&videoCodecContext_);
        videoCodecContext_ = nullptr;
        return false;
    }

    return true;
}

bool Decoder::InitAudioCodec() {
    if (audioStreamIndex_ < 0) {
        return false;
    }

    AVStream* stream = formatContext_->streams[audioStreamIndex_];
    const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);

    if (!codec) {
        std::cerr << "Unsupported audio codec" << std::endl;
        return false;
    }

    audioCodecContext_ = avcodec_alloc_context3(codec);
    if (!audioCodecContext_) {
        std::cerr << "Failed to allocate audio codec context" << std::endl;
        return false;
    }

    if (avcodec_parameters_to_context(audioCodecContext_, stream->codecpar) < 0) {
        std::cerr << "Failed to copy audio codec parameters" << std::endl;
        avcodec_free_context(&audioCodecContext_);
        audioCodecContext_ = nullptr;
        return false;
    }

    if (avcodec_open2(audioCodecContext_, codec, nullptr) < 0) {
        std::cerr << "Failed to open audio codec" << std::endl;
        avcodec_free_context(&audioCodecContext_);
        audioCodecContext_ = nullptr;
        return false;
    }

    return true;
}
