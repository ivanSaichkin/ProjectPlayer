#include "../../include/core/VideoDecoder.hpp"

#include <iostream>

VideoDecoder::VideoDecoder()
    : MediaDecoder(), codecContext(nullptr), swsContext(nullptr), videoStream(nullptr), videoStreamIndex(-1), running(false), paused(false) {
}

VideoDecoder::~VideoDecoder() {
    stop();

    if (swsContext) {
        sws_freeContext(swsContext);
        swsContext = nullptr;
    }

    if (codecContext) {
        avcodec_free_context(&codecContext);
        codecContext = nullptr;
    }
}

bool VideoDecoder::initialize() {
    std::lock_guard<std::mutex> lock(mutex);

    if (!opened || !formatContext) {
        return false;
    }

    // Find video stream
    videoStreamIndex = findStream(AVMEDIA_TYPE_VIDEO);
    if (videoStreamIndex < 0) {
        ErrorHandler::getInstance().handleError(MediaPlayerException::STREAM_ERROR, "No video stream found in the file");
        return false;
    }

    videoStream = formatContext->streams[videoStreamIndex];

    // Find decoder for the stream
    const AVCodec* codec = avcodec_find_decoder(videoStream->codecpar->codec_id);
    if (!codec) {
        ErrorHandler::getInstance().handleError(MediaPlayerException::CODEC_ERROR, "Unsupported video codec");
        return false;
    }

    // Allocate codec context
    codecContext = avcodec_alloc_context3(codec);
    if (!codecContext) {
        ErrorHandler::getInstance().handleError(MediaPlayerException::DECODER_ERROR, "Failed to allocate video codec context");
        return false;
    }

    // Copy codec parameters to context
    if (avcodec_parameters_to_context(codecContext, videoStream->codecpar) < 0) {
        ErrorHandler::getInstance().handleError(MediaPlayerException::DECODER_ERROR, "Failed to copy video codec parameters to context");
        return false;
    }

    // Open codec
    if (avcodec_open2(codecContext, codec, nullptr) < 0) {
        ErrorHandler::getInstance().handleError(MediaPlayerException::DECODER_ERROR, "Failed to open video codec");
        return false;
    }

    return true;
}

void VideoDecoder::start() {
    std::lock_guard<std::mutex> lock(mutex);

    if (running) {
        return;
    }

    running = true;
    paused = false;

    // Clear any existing frames
    while (!frameQueue.empty()) {
        frameQueue.pop();
    }

    // Start decoding thread
    decodingThread = std::thread(&VideoDecoder::decodingLoop, this);
}

void VideoDecoder::stop() {
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
    while (!frameQueue.empty()) {
        frameQueue.pop();
    }
}

bool VideoDecoder::getNextFrame(VideoFrame& frame) {
    std::lock_guard<std::mutex> lock(queueMutex);

    if (frameQueue.empty()) {
        return false;
    }

    frame = frameQueue.front();
    frameQueue.pop();

    // Notify decoding thread that a frame was consumed
    queueCondition.notify_one();

    return true;
}

sf::Vector2u VideoDecoder::getSize() const {
    if (!codecContext) {
        return sf::Vector2u(0, 0);
    }

    return sf::Vector2u(codecContext->width, codecContext->height);
}

double VideoDecoder::getFrameRate() const {
    if (!videoStream) {
        return 0.0;
    }

    if (videoStream->avg_frame_rate.den == 0) {
        return 0.0;
    }

    return static_cast<double>(videoStream->avg_frame_rate.num) / static_cast<double>(videoStream->avg_frame_rate.den);
}

bool VideoDecoder::hasMoreFrames() const {
    return running && opened;
}

void VideoDecoder::setPaused(bool paused) {
    this->paused = paused;

    if (!paused) {
        // Notify thread to continue if it was waiting
        queueCondition.notify_one();
    }
}

bool VideoDecoder::isPaused() const {
    return paused;
}

void VideoDecoder::decodingLoop() {
    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();

    if (!packet || !frame) {
        ErrorHandler::getInstance().handleError(MediaPlayerException::DECODER_ERROR, "Failed to allocate video packet or frame");

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
            if (frameQueue.size() >= MAX_QUEUE_SIZE) {
                queueCondition.wait(lock, [this] { return frameQueue.size() < MAX_QUEUE_SIZE || !running; });

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
                                                        "Error reading video frame: " + ErrorHandler::ffmpegErrorToString(readResult));
                break;
            }
        }

        // Check if this packet belongs to the video stream
        if (packet->stream_index != videoStreamIndex) {
            av_packet_unref(packet);
            continue;
        }

        // Send packet to decoder
        int sendResult = avcodec_send_packet(codecContext, packet);
        av_packet_unref(packet);

        if (sendResult < 0) {
            ErrorHandler::getInstance().handleError(MediaPlayerException::DECODER_ERROR,
                                                    "Error sending packet to video decoder: " + ErrorHandler::ffmpegErrorToString(sendResult));
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
                ErrorHandler::getInstance().handleError(MediaPlayerException::DECODER_ERROR, "Error receiving frame from video decoder: " +
                                                                                                 ErrorHandler::ffmpegErrorToString(receiveResult));
                break;
            }

            // Convert frame to texture and add to queue
            VideoFrame videoFrame;

            // Calculate presentation timestamp in seconds
            double pts = 0.0;
            if (frame->pts != AV_NOPTS_VALUE) {
                pts = frame->pts * av_q2d(videoStream->time_base);
            }

            videoFrame.pts = pts;

            if (convertFrameToTexture(frame, videoFrame.texture)) {
                std::lock_guard<std::mutex> lock(queueMutex);
                frameQueue.push(videoFrame);
            }

            av_frame_unref(frame);
        }
    }

    av_packet_free(&packet);
    av_frame_free(&frame);
}

bool VideoDecoder::convertFrameToTexture(AVFrame* frame, sf::Texture& texture) {
    // Create SwsContext if needed
    if (!swsContext) {
        swsContext = sws_getContext(codecContext->width, codecContext->height, codecContext->pix_fmt, codecContext->width, codecContext->height,
                                    AV_PIX_FMT_RGBA, SWS_BILINEAR, nullptr, nullptr, nullptr);

        if (!swsContext) {
            ErrorHandler::getInstance().handleError(MediaPlayerException::DECODER_ERROR, "Failed to create video scaling context");
            return false;
        }
    }

    // Allocate buffer for RGBA data
    uint8_t* buffer = new uint8_t[codecContext->width * codecContext->height * 4];
    if (!buffer) {
        ErrorHandler::getInstance().handleError(MediaPlayerException::DECODER_ERROR, "Failed to allocate video buffer");
        return false;
    }

    // Set up pointers for conversion
    uint8_t* dst_data[4] = {buffer, nullptr, nullptr, nullptr};
    int dst_linesize[4] = {codecContext->width * 4, 0, 0, 0};

    // Convert frame to RGBA
    sws_scale(swsContext, frame->data, frame->linesize, 0, codecContext->height, dst_data, dst_linesize);

    // Create SFML texture
    if (!texture.create(codecContext->width, codecContext->height)) {
        delete[] buffer;
        ErrorHandler::getInstance().handleError(MediaPlayerException::DECODER_ERROR, "Failed to create video texture");
        return false;
    }

    // Update texture with pixel data
    texture.update(buffer);

    // Free buffer
    delete[] buffer;

    return true;
}
