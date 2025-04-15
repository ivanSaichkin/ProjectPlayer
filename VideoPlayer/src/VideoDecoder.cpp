#include "../include/VideoDecoder.hpp"
#include <iostream>

VideoDecoder::VideoDecoder()
    : MediaDecoder(),
      codecContext(nullptr),
      swsContext(nullptr),
      videoStream(nullptr),
      videoStreamIndex(-1),
      running(false),
      paused(false) {
}

VideoDecoder::~VideoDecoder() {
    stop();

    if (codecContext) {
        avcodec_free_context(&codecContext);
    }

    if (swsContext) {
        sws_freeContext(swsContext);
        swsContext = nullptr;
    }
}

bool VideoDecoder::initialize() {
    std::lock_guard<std::mutex> lock(mutex);

    if (!isOpen()) {
        return false;
    }

    // Find video stream
    videoStreamIndex = findStream(AVMEDIA_TYPE_VIDEO);
    if (videoStreamIndex < 0) {
        std::cerr << "No video stream found" << std::endl;
        return false;
    }

    videoStream = formatContext->streams[videoStreamIndex];

    // Find decoder
    const AVCodec* codec = avcodec_find_decoder(videoStream->codecpar->codec_id);
    if (!codec) {
        std::cerr << "Unsupported video codec" << std::endl;
        return false;
    }

    // Allocate codec context
    codecContext = avcodec_alloc_context3(codec);
    if (!codecContext) {
        std::cerr << "Failed to allocate video codec context" << std::endl;
        return false;
    }

    // Copy codec parameters
    if (avcodec_parameters_to_context(codecContext, videoStream->codecpar) < 0) {
        std::cerr << "Failed to copy video codec parameters" << std::endl;
        return false;
    }

    // Open codec
    if (avcodec_open2(codecContext, codec, nullptr) < 0) {
        std::cerr << "Failed to open video codec" << std::endl;
        return false;
    }

    return true;
}

void VideoDecoder::start() {
    if (running) {
        return;
    }

    running = true;
    paused = false;

    decodingThread = std::thread(&VideoDecoder::decodingLoop, this);
}

void VideoDecoder::stop() {
    if (!running) {
        return;
    }

    running = false;
    paused = false;
    queueCondition.notify_all();

    if (decodingThread.joinable()) {
        decodingThread.join();
    }

    // Clear frame queue
    std::lock_guard<std::mutex> lock(queueMutex);
    std::queue<VideoFrame> empty;
    std::swap(frameQueue, empty);
}

bool VideoDecoder::getNextFrame(VideoFrame& frame) {
    std::unique_lock<std::mutex> lock(queueMutex);

    if (frameQueue.empty()) {
        return false;
    }

    frame = std::move(frameQueue.front());
    frameQueue.pop();

    // Notify decoding thread that we've consumed a frame
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

    if (videoStream->avg_frame_rate.den && videoStream->avg_frame_rate.num) {
        return static_cast<double>(videoStream->avg_frame_rate.num) /
               static_cast<double>(videoStream->avg_frame_rate.den);
    }

    return 0.0;
}

bool VideoDecoder::hasMoreFrames() const {
    std::lock_guard<std::mutex> lock(queueMutex);
    return !frameQueue.empty() || running;
}

void VideoDecoder::setPaused(bool pause) {
    paused = pause;
    if (!pause) {
        queueCondition.notify_one();
    }
}

bool VideoDecoder::isPaused() const {
    return paused;
}

void VideoDecoder::decodingLoop() {
    const int MAX_QUEUE_SIZE = 30;  // Limit queue size to prevent memory issues

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();

    if (!packet || !frame) {
        std::cerr << "Failed to allocate video packet or frame" << std::endl;
        av_packet_free(&packet);
        av_frame_free(&frame);
        return;
    }

    while (running) {
        // Check if queue is full
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            if (frameQueue.size() >= MAX_QUEUE_SIZE) {
                queueCondition.wait(lock, [this, MAX_QUEUE_SIZE]() {
                    return frameQueue.size() < MAX_QUEUE_SIZE || !running;
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

        // Process video packets
        if (packet->stream_index == videoStreamIndex) {
            // Send packet to decoder
            ret = avcodec_send_packet(codecContext, packet);
            if (ret < 0) {
                std::cerr << "Error sending packet to video decoder" << std::endl;
                av_packet_unref(packet);
                continue;
            }

            // Receive frames
            while (ret >= 0 && running) {
                ret = avcodec_receive_frame(codecContext, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                } else if (ret < 0) {
                    std::cerr << "Error receiving frame from video decoder" << std::endl;
                    break;
                }

                // Convert frame to texture and add to queue
                VideoFrame videoFrame;

                // Calculate PTS in seconds
                double pts = 0.0;
                if (frame->pts != AV_NOPTS_VALUE) {
                    pts = frame->pts * av_q2d(videoStream->time_base);
                }

                videoFrame.pts = pts;

                if (convertFrameToTexture(frame, videoFrame.texture)) {
                    std::lock_guard<std::mutex> lock(queueMutex);
                    frameQueue.push(std::move(videoFrame));
                }
            }
        }

        av_packet_unref(packet);
    }

    av_packet_free(&packet);
    av_frame_free(&frame);
}

bool VideoDecoder::convertFrameToTexture(AVFrame* frame, sf::Texture& texture) {
    // Initialize swscale context if needed
    if (!swsContext) {
        swsContext = sws_getContext(
            codecContext->width, codecContext->height, codecContext->pix_fmt,
            codecContext->width, codecContext->height, AV_PIX_FMT_RGBA,
            SWS_BILINEAR, nullptr, nullptr, nullptr
        );

        if (!swsContext) {
            std::cerr << "Failed to initialize swscale context" << std::endl;
            return false;
        }
    }

    // Create RGBA frame
    AVFrame* rgbaFrame = av_frame_alloc();
    if (!rgbaFrame) {
        std::cerr << "Failed to allocate RGBA frame" << std::endl;
        return false;
    }

    // Allocate buffer for RGBA frame
    int bufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGBA, codecContext->width, codecContext->height, 1);
    uint8_t* buffer = static_cast<uint8_t*>(av_malloc(bufferSize));
    if (!buffer) {
        std::cerr << "Failed to allocate RGBA buffer" << std::endl;
        av_frame_free(&rgbaFrame);
        return false;
    }

    // Set up RGBA frame
    av_image_fill_arrays(rgbaFrame->data, rgbaFrame->linesize, buffer, AV_PIX_FMT_RGBA,
                         codecContext->width, codecContext->height, 1);

    // Convert frame to RGBA
    sws_scale(swsContext, frame->data, frame->linesize, 0, codecContext->height,
              rgbaFrame->data, rgbaFrame->linesize);

    // Create SFML texture
    if (!texture.create(codecContext->width, codecContext->height)) {
        std::cerr << "Failed to create SFML texture" << std::endl;
        av_freep(&buffer);
        av_frame_free(&rgbaFrame);
        return false;
    }

    // Update texture with RGBA data
    texture.update(buffer);

    // Clean up
    av_freep(&buffer);
    av_frame_free(&rgbaFrame);

    return true;
}
