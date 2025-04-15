# Video Player Backend API Documentation

This documentation covers the backend API for a video player system built with C++ using FFmpeg and SFML.

## Table of Contents
- [Overview](#overview)
- [Architecture](#architecture)
- [Core Components](#core-components)
- [API Reference](#api-reference)
- [Integration Guide](#integration-guide)
- [Threading Model](#threading-model)
- [Error Handling](#error-handling)
- [Build Instructions](#build-instructions)

## Overview
The Video Player Backend API provides:
- Video/audio decoding using FFmpeg
- Frame-accurate seeking
- Volume control
- Playback state management
- Event callbacks
- Thread-safe operation

## Architecture
![image](https://github.com/user-attachments/assets/29888c8f-c230-499e-a7c4-31ce014cc8a5)


## Core Components
- `MediaPlayer`: Main controller
- `VideoDecoder`: Video stream handling
- `AudioDecoder`: Audio stream handling
- `MediaDecoder`: Base decoder class
- `ErrorHandler`: Error management

## API Reference

### MediaPlayer
```cpp
// Control methods
bool open(const std::string& filename);
void close();
void play();
void pause();
void togglePlayPause();
void seek(double seconds);
void setVolume(float volume);

// Status methods
bool isPlaying() const;
double getDuration() const;
double getCurrentPosition() const;
sf::Vector2u getVideoSize() const;

// Frame access
bool getCurrentFrame(sf::Texture& texture);
void update();

// Callbacks
void setPlaybackStartCallback(std::function<void()> callback);
void setErrorCallback(std::function<void(const MediaPlayerException&)> callback);
```

### VideoDecoder
```cpp
bool initialize();
void start();
void stop();
bool getNextFrame(VideoFrame& frame);
sf::Vector2u getSize() const;
```

### AudioDecoder
```cpp
bool initialize();
void start();
void stop();
bool getNextPacket(AudioPacket& packet);
unsigned int getSampleRate() const;
```
## Integration Guide
```cpp
MediaPlayer player;
player.setErrorCallback([](const MediaPlayerException& e) {
    std::cerr << "Error: " << e.what() << std::endl;
});

if (!player.open("video.mp4")) {
    std::cerr << "Failed to open file" << std::endl;
    return;
}

player.play();

while (running) {
    player.update();
    sf::Texture frame;
    if (player.getCurrentFrame(frame)) {
        // Render frame
    }
}
```
## Threading Model

- **Main thread**: Handles API calls and player updates  
- **Video thread**: Dedicated to frame decoding  
- **Audio thread**: Handles packet decoding  
- **SFML thread**: Manages audio playback  

## Error Handling

```cpp
enum ErrorCode {
    FILE_NOT_FOUND,
    DECODER_ERROR,
    FORMAT_ERROR,
    CODEC_ERROR,
    STREAM_ERROR,
    UNKNOWN_ERROR
};
```
## Build Instructions

### Prerequisites

- **Compiler**: C++17 compatible
- **Build System**: CMake 3.10+

#### FFmpeg Libraries:
- libavcodec
- libavformat  
- libavutil  
- libswscale  
- libswresample  

#### SFML 2.5+:
- sfml-graphics
- sfml-window  
- sfml-system  
- sfml-audio
