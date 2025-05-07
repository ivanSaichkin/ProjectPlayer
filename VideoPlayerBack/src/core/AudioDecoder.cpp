#include "../../include/core/AudioDecoder.hpp"

#include <iostream>

AudioDecoder::AudioDecoder()
    : MediaDecoder(), codecContext(nullptr), swrContext(nullptr), audioStream(nullptr), audioStreamIndex(-1), running(false), paused(false) {
}

AudioDecoder::~AudioDecoder() {
    stop();

    if (swrContext) {
        swr_free(&swrContext);
        swrContext = nullptr;
    }

    if (codecContext) {
        avcodec_free_context(&codecContext);
        codecContext = nullptr;
    }
}

bool AudioDecoder::initialize() {
    std::lock_guard<std::mutex> lock(mutex);

    if (!opened || !formatContext) {
        return false;
    }

    // Find audio stream
    audioStreamIndex = findStream(AVMEDIA_TYPE_AUDIO);
    if (audioStreamIndex < 0) {
        // No audio stream, not an error but return false
        return false;
    }

    audioStream = formatContext->streams[audioStreamIndex];

    // Find decoder for the stream
    const AVCodec* codec = avcodec_find_decoder(audioStream->codecpar->codec_id);
    if (!codec) {
        ErrorHandler::getInstance().handleError(MediaPlayerException::CODEC_ERROR, "Unsupported audio codec");
        return false;
    }

    // Allocate codec context
    codecContext = avcodec_alloc_context3(codec);
    if (!codecContext) {
        ErrorHandler::getInstance().handleError(MediaPlayerException::DECODER_ERROR, "Failed to allocate audio codec context");
        return false;
    }

    // Copy codec parameters to context
    if (avcodec_parameters_to_context(codecContext, audioStream->codecpar) < 0) {
        ErrorHandler::getInstance().handleError(MediaPlayerException::DECODER_ERROR, "Failed to copy audio codec parameters to context");
        return false;
    }

    // Open codec
    if (avcodec_open2(codecContext, codec, nullptr) < 0) {
        ErrorHandler::getInstance().handleError(MediaPlayerException::DECODER_ERROR, "Failed to open audio codec");
        return false;
    }

    // Create resampler context
    swrContext = swr_alloc();
    if (!swrContext) {
        ErrorHandler::getInstance().handleError(MediaPlayerException::DECODER_ERROR, "Failed to allocate audio resampler context");
        return false;
    }

    // Set resampler options
    av_opt_set_int(swrContext, "in_channel_layout", codecContext->channel_layout, 0);
    av_opt_set_int(swrContext, "out_channel_layout", AV_CH_LAYOUT_STEREO, 0);
    av_opt_set_int(swrContext, "in_sample_rate", codecContext->sample_rate, 0);
    av_opt_set_int(swrContext, "out_sample_rate", 44100, 0);
    av_opt_set_sample_fmt(swrContext, "in_sample_fmt", codecContext->sample_fmt, 0);
    av_opt_set_sample_fmt(swrContext, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);

    // Initialize resampler
    if (swr_init(swrContext) < 0) {
        ErrorHandler::getInstance().handleError(MediaPlayerException::DECODER_ERROR, "Failed to initialize audio resampler");
        return false;
    }

    return true;
}

void AudioDecoder::start() {
    std::lock_guard<std::mutex> lock(mutex);

    if (running) {
        return;
    }

    running = true;
    paused = false;

    // Clear any existing packets
    while (!packetQueue.empty()) {
        packetQueue.pop();
    }

    // Start decoding thread
    decodingThread = std::thread(&AudioDecoder::decodingLoop, this);
}

void AudioDecoder::stop() {
    {
        std::lock_guard<std::mutex> lock(mutex);

        if (!running) {
            return;
        }

        running = false;
        paused = false;
    }

    // Notify condition variable to wake up thread
    queueCondition.notify_all();

    // Wait for thread to finish
    if (decodingThread.joinable()) {
        decodingThread.join();
    }

    // Clear queue
    std::lock_guard<std::mutex> lock(queueMutex);
    while (!packetQueue.empty()) {
        packetQueue.pop();
    }
}

bool AudioDecoder::getNextPacket(AudioPacket& packet) {
    std::lock_guard<std::mutex> lock(queueMutex);

    if (packetQueue.empty()) {
        return false;
    }

    packet = packetQueue.front();
    packetQueue.pop();

    // Notify decoding thread that a packet was consumed
    queueCondition.notify_one();

    return true;
}

unsigned int AudioDecoder::getSampleRate() const {
    if (!codecContext) {
        return 44100;  // Default sample rate
    }

    return 44100;  // We're resampling to 44100 Hz
}

unsigned int AudioDecoder::getChannelCount() const {
    if (!codecContext) {
        return 2;  // Default to stereo
    }

    return 2;  // We're resampling to stereo
}

bool AudioDecoder::hasMorePackets() const {
    return running && opened;
}

void AudioDecoder::setPaused(bool paused) {
    this->paused = paused;

    if (!paused) {
        // Notify thread to continue if it was waiting
        queueCondition.notify_one();
    }
}

bool AudioDecoder::isPaused() const {
    return paused;
}

void AudioDecoder::decodingLoop() {
    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();

    if (!packet || !frame) {
        ErrorHandler::getInstance().handleError(MediaPlayerException::DECODER_ERROR, "Failed to allocate audio packet or frame");

        if (packet)
            av_packet_free(&packet);
        if (frame)
            av_frame_free(&frame);

        return;
    }

    while (running) {
        // Check if paused
        if (paused) {
            std::unique_lock<std::mutex> lock(mutex);
            queueCondition.wait(lock, [this] { return !paused || !running; });

            if (!running) {
                break;
            }

            continue;
        }

        // Check if queue is full
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            if (packetQueue.size() >= MAX_QUEUE_SIZE) {
                queueCondition.wait(lock, [this] { return packetQueue.size() < MAX_QUEUE_SIZE || !running; });

                if (!running) {
                    break;
                }

                continue;
            }
        }

        // Read packet
        int readResult;
        {
            std::lock_guard<std::mutex> lock(mutex);

            if (!formatContext) {
                break;
            }

            readResult = av_read_frame(formatContext, packet);
        }

        if (readResult < 0) {
            // End of file or error
            if (readResult == AVERROR_EOF) {
                // End of file, loop back to beginning
                seek(0);
                continue;
            } else {
                // Error
                ErrorHandler::getInstance().handleError(MediaPlayerException::DECODER_ERROR,
                                                        "Error reading audio frame: " + ErrorHandler::ffmpegErrorToString(readResult));
                break;
            }
        }

        // Check if this packet belongs to the audio stream
        if (packet->stream_index != audioStreamIndex) {
            av_packet_unref(packet);
            continue;
        }

        // Send packet to decoder
        int sendResult = avcodec_send_packet(codecContext, packet);
        av_packet_unref(packet);

        if (sendResult < 0) {
            ErrorHandler::getInstance().handleError(MediaPlayerException::DECODER_ERROR,
                                                    "Error sending packet to audio decoder: " + ErrorHandler::ffmpegErrorToString(sendResult));
            break;
        }

        // Receive frames from decoder
        while (running) {
            int receiveResult = avcodec_receive_frame(codecContext, frame);

            if (receiveResult == AVERROR(EAGAIN) || receiveResult == AVERROR_EOF) {
                // Need more packets or end of stream
                break;
            } else if (receiveResult < 0) {
                // Error
                ErrorHandler::getInstance().handleError(MediaPlayerException::DECODER_ERROR, "Error receiving frame from audio decoder: " +
                                                                                                 ErrorHandler::ffmpegErrorToString(receiveResult));
                break;
            }

            // Convert frame to audio samples and add to queue
            AudioPacket audioPacket;

            // Calculate presentation timestamp in seconds
            double pts = 0.0;
            if (frame->pts != AV_NOPTS_VALUE) {
                pts = frame->pts * av_q2d(audioStream->time_base);
            }

            audioPacket.pts = pts;

            if (convertFrameToSamples(frame, audioPacket.samples)) {
                std::lock_guard<std::mutex> lock(queueMutex);
                packetQueue.push(audioPacket);
            }

            av_frame_unref(frame);
        }
    }

    av_packet_free(&packet);
    av_frame_free(&frame);
}

bool AudioDecoder::convertFrameToSamples(AVFrame* frame, std::vector<sf::Int16>& samples) {
    // Calculate the number of samples to output
    int outSamples =
        av_rescale_rnd(swr_get_delay(swrContext, codecContext->sample_rate) + frame->nb_samples, 44100, codecContext->sample_rate, AV_ROUND_UP);

    // Allocate buffer for resampled data
    samples.resize(outSamples * 2);  // 2 channels (stereo)

    // Resample
    uint8_t* outBuffer = reinterpret_cast<uint8_t*>(samples.data());
    int samplesResampled = swr_convert(swrContext, &outBuffer, outSamples, (const uint8_t**)frame->data, frame->nb_samples);

    if (samplesResampled < 0) {
        ErrorHandler::getInstance().handleError(MediaPlayerException::DECODER_ERROR,
                                                "Error resampling audio: " + ErrorHandler::ffmpegErrorToString(samplesResampled));
        return false;
    }

    // Resize to actual number of samples
    samples.resize(samplesResampled * 2);

    return true;
}
