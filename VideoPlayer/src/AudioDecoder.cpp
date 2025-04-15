#include "../include/AudioDecoder.hpp"
#include <iostream>

AudioDecoder::AudioDecoder()
    : MediaDecoder(),
      codecContext(nullptr),
      swrContext(nullptr),
      audioStream(nullptr),
      audioStreamIndex(-1),
      running(false),
      paused(false) {
}

AudioDecoder::~AudioDecoder() {
    stop();

    if (codecContext) {
        avcodec_free_context(&codecContext);
    }

    if (swrContext) {
        swr_free(&swrContext);
    }
}

bool AudioDecoder::initialize() {
    std::lock_guard<std::mutex> lock(mutex);

    if (!isOpen()) {
        return false;
    }

    // Find audio stream
    audioStreamIndex = findStream(AVMEDIA_TYPE_AUDIO);
    if (audioStreamIndex < 0) {
        std::cerr << "No audio stream found" << std::endl;
        return false;
    }

    audioStream = formatContext->streams[audioStreamIndex];

    // Find decoder
    const AVCodec* codec = avcodec_find_decoder(audioStream->codecpar->codec_id);
    if (!codec) {
        std::cerr << "Unsupported audio codec" << std::endl;
        return false;
    }

    // Allocate codec context
    codecContext = avcodec_alloc_context3(codec);
    if (!codecContext) {
        std::cerr << "Failed to allocate audio codec context" << std::endl;
        return false;
    }

    // Copy codec parameters
    if (avcodec_parameters_to_context(codecContext, audioStream->codecpar) < 0) {
        std::cerr << "Failed to copy audio codec parameters" << std::endl;
        return false;
    }

    // Open codec
    if (avcodec_open2(codecContext, codec, nullptr) < 0) {
        std::cerr << "Failed to open audio codec" << std::endl;
        return false;
    }

    // Initialize resampler
    swrContext = swr_alloc_set_opts(nullptr,
        AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, 44100,  // Output
        codecContext->channel_layout, codecContext->sample_fmt, codecContext->sample_rate,  // Input
        0, nullptr);

    if (!swrContext || swr_init(swrContext) < 0) {
        std::cerr << "Failed to initialize audio resampler" << std::endl;
        return false;
    }

    return true;
}

void AudioDecoder::start() {
    if (running) {
        return;
    }

    running = true;
    paused = false;

    decodingThread = std::thread(&AudioDecoder::decodingLoop, this);
}

void AudioDecoder::stop() {
    if (!running) {
        return;
    }

    running = false;
    paused = false;
    queueCondition.notify_all();

    if (decodingThread.joinable()) {
        decodingThread.join();
    }

    // Clear packet queue
    std::lock_guard<std::mutex> lock(queueMutex);
    std::queue<AudioPacket> empty;
    std::swap(packetQueue, empty);
}

bool AudioDecoder::getNextPacket(AudioPacket& packet) {
    std::unique_lock<std::mutex> lock(queueMutex);

    if (packetQueue.empty()) {
        return false;
    }

    packet = std::move(packetQueue.front());
    packetQueue.pop();

    // Notify decoding thread that we've consumed a packet
    queueCondition.notify_one();

    return true;
}

unsigned int AudioDecoder::getSampleRate() const {
    return 44100;  // We're resampling to 44100Hz
}

unsigned int AudioDecoder::getChannelCount() const {
    return 2;  // We're resampling to stereo
}

bool AudioDecoder::hasMorePackets() const {
    std::lock_guard<std::mutex> lock(queueMutex);
    return !packetQueue.empty() || running;
}

void AudioDecoder::setPaused(bool pause) {
    paused = pause;
    if (!pause) {
        queueCondition.notify_one();
    }
}

bool AudioDecoder::isPaused() const {
    return paused;
}

void AudioDecoder::decodingLoop() {
    const int MAX_QUEUE_SIZE = 30;  // Limit queue size to prevent memory issues

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();

    if (!packet || !frame) {
        std::cerr << "Failed to allocate audio packet or frame" << std::endl;
        av_packet_free(&packet);
        av_frame_free(&frame);
        return;
    }

    while (running) {
        // Check if queue is full
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            if (packetQueue.size() >= MAX_QUEUE_SIZE) {
                queueCondition.wait(lock, [this, MAX_QUEUE_SIZE]() {
                    return packetQueue.size() < MAX_QUEUE_SIZE || !running;
                });

                if (!running) {
                    break;
                }
            }

            // Check if paused
            if (paused) {
                queueCondition.wait(lock, [this]() {
                    return !paused || !running;
                });

                if (!running) {
                    break;
                }
            }
        }

        // Read packet
        int ret = av_read_frame(formatContext, packet);
        if (ret < 0) {
            // End of file or error
            if (ret == AVERROR_EOF) {
                // End of file, wait for a seek or stop
                std::unique_lock<std::mutex> lock(queueMutex);
                queueCondition.wait(lock, [this]() {
                    return !running;
                });
            }
            break;
        }

        // Process audio packets
        if (packet->stream_index == audioStreamIndex) {
            // Send packet to decoder
            ret = avcodec_send_packet(codecContext, packet);
            if (ret < 0) {
                std::cerr << "Error sending packet to audio decoder" << std::endl;
                av_packet_unref(packet);
                continue;
            }

            // Receive frames
            while (ret >= 0 && running) {
                ret = avcodec_receive_frame(codecContext, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                } else if (ret < 0) {
                    std::cerr << "Error receiving frame from audio decoder" << std::endl;
                    break;
                }

                // Convert frame to audio samples and add to queue
                AudioPacket audioPacket;

                // Calculate PTS in seconds
                double pts = 0.0;
                if (frame->pts != AV_NOPTS_VALUE) {
                    pts = frame->pts * av_q2d(audioStream->time_base);
                }

                audioPacket.pts = pts;

                if (convertFrameToSamples(frame, audioPacket.samples)) {
                    std::lock_guard<std::mutex> lock(queueMutex);
                    packetQueue.push(std::move(audioPacket));
                }
            }
        }

        av_packet_unref(packet);
    }

    av_packet_free(&packet);
    av_frame_free(&frame);
}

bool AudioDecoder::convertFrameToSamples(AVFrame* frame, std::vector<sf::Int16>& samples) {
    // Calculate output buffer size
    int outSamples = av_rescale_rnd(
        swr_get_delay(swrContext, codecContext->sample_rate) + frame->nb_samples,
        44100,
        codecContext->sample_rate,
        AV_ROUND_UP
    );

    // Allocate output buffer
    samples.resize(outSamples * 2);  // 2 channels (stereo)

    // Convert audio
    uint8_t* outBuffer = reinterpret_cast<uint8_t*>(samples.data());
    int samplesConverted = swr_convert(
        swrContext,
        &outBuffer, outSamples,
        (const uint8_t**)frame->data, frame->nb_samples
    );

    if (samplesConverted < 0) {
        std::cerr << "Error converting audio samples" << std::endl;
        return false;
    }

    // Resize to actual converted size
    samples.resize(samplesConverted * 2);

    return true;
}
