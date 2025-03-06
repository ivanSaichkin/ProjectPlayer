#include "../include/AudioDecoder.hpp"

AudioDecoder::AudioDecoder(const MediaFile& mediaFile) : mediaFile_(mediaFile) {
    AVCodecParameters* audioCodecParameters = mediaFile_.GetFormatContext()->streams[mediaFile_.GetAudioStreamIndex()]->codecpar;
    const AVCodec* audioCodec = avcodec_find_decoder(audioCodecParameters->codec_id);
    if (!audioCodec) {
        throw std::runtime_error("Unsupported audio codec");
    }

    audioCodecContext_ = avcodec_alloc_context3(audioCodec);
    avcodec_parameters_to_context(audioCodecContext_, audioCodecParameters);

    if (avcodec_open2(audioCodecContext_, audioCodec, nullptr) < 0) {
        throw std::runtime_error("Failed to open audio codec");
    }

    soundBuffer_.loadFromSamples(nullptr, 0, audioCodecContext_->sample_rate, audioCodecContext_->channels);
    sound_.setBuffer(soundBuffer_);
}

void AudioDecoder::Start() {
    isRunning_ = true;
    std::thread([this]() { DecodeAudio(); }).detach();
}

void AudioDecoder::Stop() {
    isRunning_ = false;
}

void AudioDecoder::DecodeAudio() {
    AVPacket* packet = av_packet_alloc();

    while (isRunning_) {
        if (av_read_frame(mediaFile_.GetFormatContext(), packet) >= 0) {
            if (packet->stream_index == mediaFile_.GetAudioStreamIndex()) {
                if (avcodec_send_packet(audioCodecContext_, packet) == 0) {
                    AVFrame* audioFrame = av_frame_alloc();

                    while (avcodec_receive_frame(audioCodecContext_, audioFrame) == 0) {
                        std::vector<int16_t> samples(audioFrame->nb_samples * audioFrame->channels);

                        for (int i = 0; i < audioFrame->nb_samples; ++i) {
                            for (int j = 0; j < audioFrame->channels; ++j) {
                                samples[i * audioFrame->channels + j] = ((int16_t*)audioFrame->data[j])[i];
                            }
                        }

                        soundBuffer_.loadFromSamples(samples.data(), samples.size(), audioFrame->channels, audioFrame->sample_rate);
                        sound_.play();
                    }
                    av_frame_free(&audioFrame);
                }
            }
            av_packet_unref(packet);
        } else {
            isRunning_ = false;
        }
    }
    av_packet_free(&packet);
}
