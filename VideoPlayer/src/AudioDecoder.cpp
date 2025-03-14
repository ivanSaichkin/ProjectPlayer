#include "../include/AudioDecoder.hpp"

AudioDecoder::AudioDecoder(const MediaFile& mediaFile) : mediaFile_(mediaFile), audioCodecContext_(nullptr) {
    isPaused_ = false;
    isRunning_ = false;

    AVCodecParameters* codecParams = mediaFile_.GetFormatContext()->streams[mediaFile_.GetAudioStreamIndex()]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);

    if (!codec)
        throw std::runtime_error("Unsupported audio codec");

    audioCodecContext_ = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(audioCodecContext_, codecParams);

    if (avcodec_open2(audioCodecContext_, codec, nullptr) < 0)
        throw std::runtime_error("Failed to open audio codec");

    soundBuffer_.loadFromSamples(nullptr, 0, audioCodecContext_->sample_rate, audioCodecContext_->channels);
    sound_.setBuffer(soundBuffer_);
}

AudioDecoder::~AudioDecoder() {
    Stop();

    if (audioCodecContext_) {
        avcodec_free_context(&audioCodecContext_);
    }
}

void AudioDecoder::Start() {
    isRunning_ = true;
    std::thread([this]() { DecodeAudio(); }).detach();
}

void AudioDecoder::Stop() {
    isRunning_ = false;
}

void AudioDecoder::SetVolume(float volume) {
    sound_.setVolume(volume);
}

void AudioDecoder::TogglePause() {
    isRunning_ = !isRunning_;
}

void AudioDecoder::DecodeAudio() {
    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    std::unique_lock<std::mutex> lock(mutex_, std::defer_lock);

    if (!packet || !frame) {
        throw std::runtime_error("Failed to allocate FFmpeg structures");
    }

    // Рассчёт длительности сэмпла в микросекундах
    const double time_per_sample = 1.0 / audioCodecContext_->sample_rate;
    auto start_time = std::chrono::steady_clock::now();

    while (isRunning_) {
        if (isPaused_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        int read_result = av_read_frame(mediaFile_.GetFormatContext(), packet);
        if (read_result < 0) {
            if (read_result == AVERROR_EOF) {
                // Достигнут конец файла
                isRunning_ = false;
            }
            continue;
        }

        if (packet->stream_index == mediaFile_.GetAudioStreamIndex()) {
            if (avcodec_send_packet(audioCodecContext_, packet) != 0) {
                av_packet_unref(packet);
                continue;
            }

            while (avcodec_receive_frame(audioCodecContext_, frame) == 0) {
                // Получаем временную метку
                double pts = frame->best_effort_timestamp * mediaFile_.GetAudioTimeBase();

                // Расчёт задержки для синхронизации
                auto target_time = start_time + std::chrono::duration<double>(pts);
                std::this_thread::sleep_until(target_time);

                // Конвертация данных
                std::vector<int16_t> samples(frame->nb_samples * frame->channels);
                for (int i = 0; i < frame->nb_samples; i++) {
                    for (int ch = 0; ch < frame->channels; ch++) {
                        samples[i * frame->channels + ch] = reinterpret_cast<int16_t*>(frame->data[ch])[i];
                    }
                }

                // Блокировка мьютекса для обновления буфера
                lock.lock();
                soundBuffer_.loadFromSamples(samples.data(), samples.size(), frame->channels, audioCodecContext_->sample_rate);
                sound_.setBuffer(soundBuffer_);
                sound_.play();
                lock.unlock();
            }
        }
        av_packet_unref(packet);
    }

    av_packet_free(&packet);
    av_frame_free(&frame);

    // Остановка воспроизведения при завершении
    sound_.stop();
}

void AudioDecoder::Flush() {
    avcodec_flush_buffers(audioCodecContext_);
}

void AudioDecoder::SetStartTime(std::chrono::steady_clock::time_point startTime) {
    startTime_ = startTime;
}
