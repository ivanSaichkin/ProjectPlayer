# Video Player Core Library Documentation

## Overview
This documentation covers the core components of the Video Player library, focusing on the media decoding and playback functionality without UI elements. The library provides a robust foundation for building media players with custom user interfaces.

## Core Components

### 1. AudioDecoder
The `AudioDecoder` class is responsible for decoding audio streams from media files.

#### Key Features:
- Decodes audio streams using FFmpeg
- Converts audio to standard PCM format
- Provides seeking capabilities
- Extracts audio metadata (sample rate, channels, codec)

#### Usage Example:

```cpp
#include "core/AudioDecoder.hpp"

VideoPlayer::Core::AudioDecoder decoder;
if (decoder.open("audio.mp3")) {
    std::vector<short> samples;
    int numSamples = decoder.getSampleRate() / 10; // 0.1 second of audio

    while (decoder.decodeAudioSamples(samples, numSamples)) {
        // Process audio samples
        // ...
    }

    decoder.close();
}
```

### Key Methods:

- **`bool open(const std::string& filePath)`**
  Opens an audio file for decoding.

- **`void close()`**
  Closes the decoder and releases resources.

- **`bool decodeAudioSamples(std::vector<short>& samples, int numSamples)`**
  Decodes the specified number of audio samples.

- **`bool seek(double position)`**
  Seeks to the specified position in seconds.

- **`int getSampleRate()`**
  Returns the sample rate of the audio stream.

- **`int getChannels()`**
  Returns the number of audio channels.

- **`double getDuration()`**
  Returns the duration of the audio stream in seconds.

## 2. VideoDecoder

The `VideoDecoder` class is responsible for decoding video streams from media files.

### Key Features:

- Decodes video streams using **FFmpeg**
- Converts frames to **RGBA format** for easy rendering
- Provides **seeking capabilities**
- Extracts video metadata:
  - Dimensions (width/height)
  - Frame rate
  - Codec information

```cpp
#include "core/VideoDecoder.hpp"

VideoPlayer::Core::VideoDecoder decoder;
if (decoder.open("video.mp4")) {
    while (decoder.decodeNextFrame()) {
        // Access frame data
        const uint8_t* frameData = decoder.getFrameData();
        int width = decoder.getWidth();
        int height = decoder.getHeight();

        // Render frame
        // ...

        // Wait for next frame time
        // ...
    }

    decoder.close();
}
```
### Key Methods:

- **`bool open(const std::string& filePath)`**
  Opens a video file for decoding.
  *Returns `true` if successful.*

- **`void close()`**
  Closes the decoder and releases all resources.

- **`bool decodeNextFrame()`**
  Decodes the next video frame.
  *Returns `true` if a frame was successfully decoded.*

- **`bool seek(double position)`**
  Seeks to the specified position (in seconds).
  *Returns `true` if seek was successful.*

- **`const uint8_t* getFrameData()`**
  Returns a pointer to the decoded frame data in RGBA format.

- **`int getWidth()`**
  Returns the width of the video frames in pixels.

- **`int getHeight()`**
  Returns the height of the video frames in pixels.

- **`double getFrameRate()`**
  Returns the frame rate (in frames per second) of the video stream.

- **`double getDuration()`**
  Returns the total duration of the video stream (in seconds).


## 3. MediaPlayer

The `MediaPlayer` class integrates audio and video decoding to provide synchronized media playback.

### Key Features:
- **Synchronized audio and video playback**
- **Comprehensive playback control**:
  - Play/Pause/Stop
  - Seek to position
  - Frame-step navigation
- **Event callback system** for player state changes
- **Volume control** (0-100% range)
- **Playback speed adjustment** (variable speed support)
- **Automatic A/V synchronization**
- **Hardware acceleration support** (where available)

```cpp
#include "core/MediaPlayer.hpp"

VideoPlayer::Core::MediaPlayer player;

// Set up callbacks
player.setOnFrameCallback([](const uint8_t* frameData, int width, int height) {
    // Render video frame
    // ...
});

player.setOnStateChangedCallback([](VideoPlayer::Core::MediaPlayer::State state) {
    // Handle state change
    // ...
});

// Open and play media file
if (player.open("media.mp4")) {
    player.play();

    // Wait for playback to finish
    // ...

    player.stop();
    player.close();
}
```

## 3. MediaPlayer

The `MediaPlayer` class provides synchronized audio/video playback with comprehensive controls.

### Key Methods

#### Playback Control
- **`bool open(const std::string& filePath)`**
  Opens a media file for playback. Returns `true` on success.

- **`void close()`**
  Closes the media file and releases all resources.

- **`void play()`**
  Starts or resumes playback from current position.

- **`void pause()`**
  Pauses playback at current position.

- **`void stop()`**
  Stops playback and resets to beginning.

#### Playback Navigation
- **`void seek(double position)`**
  Seeks to specified position (in seconds).

- **`double getPosition()`**
  Returns current playback position (in seconds).

- **`double getDuration()`**
  Returns total media duration (in seconds).

#### Audio Control
- **`void setVolume(float volume)`**
  Sets playback volume (0 = silent, 100 = max).

#### Playback Configuration
- **`void setPlaybackSpeed(float speed)`**
  Sets playback speed (0.5 = half speed, 2.0 = double speed).

#### State Management
- **`State getState()`**
  Returns current player state (see States enum below).

#### Callbacks
- **`void setOnFrameCallback(OnFrameCallback callback)`**
  Sets callback for new video frames (RGBA format).

- **`void setOnStateChangedCallback(OnStateChangedCallback callback)`**
  Sets callback for player state changes.

### States Enum
```cpp
enum State {
    Stopped,      // Playback is stopped
    Playing,      // Media is playing
    Paused,       // Playback is paused
    EndOfMedia,   // End of media reached
    Error         // Error occurred during playback
};
```

## 4. MediaPlayerException

The `MediaPlayerException` class provides comprehensive error handling for the media player library.

### Key Features:
- **Detailed error codes** and descriptive messages
- **Error categorization** for handling different failure types
- **Stack trace information** (when available)
- **Inherits from `std::runtime_error`** for standard exception handling

### Key Methods:

#### Error Information
- **`const char* what() const noexcept`**
  Returns a human-readable description of the error
  *Overrides std::runtime_error::what()*

- **`int getCode() const noexcept`**
  Returns the specific error code for programmatic handling

- **`ErrorCategory getCategory() const noexcept`**
  Returns the error category (see categories below)

  ### Thread Safety
The core components are designed with thread safety in mind:

- **AudioDecoder**: Thread-safe for all public methods
- **VideoDecoder**: Thread-safe for all public methods
- **MediaPlayer**: Thread-safe for all public methods

Each component uses internal mutexes to protect shared state during concurrent access.

### Error Handling
The library uses exceptions for error handling. All components throw `MediaPlayerException` instances when errors occur. It's recommended to wrap media player operations in try-catch blocks to handle these exceptions.

### Performance Considerations

#### Memory Usage
- **AudioDecoder**: Maintains an internal buffer of decoded audio samples
- **VideoDecoder**: Keeps one decoded frame in memory at a time
- **MediaPlayer**: Uses separate threads for audio and video decoding, which increases memory usage but improves performance

#### CPU Usage
- Decoding high-resolution video can be CPU-intensive
- The library uses FFmpeg's hardware acceleration when available
- Consider using lower resolution media on resource-constrained devices

### Integration Guide

#### Dependencies
The library depends on the following external libraries:

**FFmpeg**: For audio and video decoding
- libavformat
- libavcodec
- libswscale
- libswresample
- libavutil

**Standard Library**: C++11 or later
- std::thread
- std::mutex
- std::atomic
- std::vector
- std::function

### Integration with Custom UI

1. **Video Rendering Setup**
   - Create a rendering surface for video frames
   - Implement frame presentation in your graphics pipeline

2. **Audio Output Configuration**
   - Initialize your preferred audio API
   - Configure audio sample delivery callback

3. **Player Initialization**
   - Create MediaPlayer instance
   - Set up frame and state change callbacks

4. **UI Controls Binding**
   - Connect play/pause buttons to corresponding methods
   - Bind seek bar to seek() and getPosition()
   - Wire volume controls to setVolume()

5. **Event Handling**
   - Implement callback handlers for state changes
   - Process frame updates in rendering thread
   - Handle error conditions through exception catching

#### Usage Example:

```cpp
#include "core/MediaPlayer.hpp"
#include "YourUI/RenderingSurface.hpp"
#include "YourUI/AudioOutput.hpp"

class MediaPlayerUI {
public:
    MediaPlayerUI() {
        // Set up callbacks
        player.setOnFrameCallback([this](const uint8_t* frameData, int width, int height) {
            renderingSurface.renderFrame(frameData, width, height);
        });

        player.setOnStateChangedCallback([this](VideoPlayer::Core::MediaPlayer::State state) {
            updateUIState(state);
        });
    }

    void openFile(const std::string& filePath) {
        if (player.open(filePath)) {
            updateUIWithMediaInfo();
            player.play();
        }
    }

    void playPause() {
        if (player.getState() == VideoPlayer::Core::MediaPlayer::Playing) {
            player.pause();
        } else {
            player.play();
        }
    }

    void stop() {
        player.stop();
    }

    void seek(double position) {
        player.seek(position);
    }

    void setVolume(float volume) {
        player.setVolume(volume);
    }

private:
    VideoPlayer::Core::MediaPlayer player;
    RenderingSurface renderingSurface;
    AudioOutput audioOutput;

    void updateUIWithMediaInfo() {
        // Update UI with media information
        // ...
    }

    void updateUIState(VideoPlayer::Core::MediaPlayer::State state) {
        // Update UI based on player state
        // ...
    }
};
```

### Best Practices

#### Resource Management
- Always call `close()` when done with a decoder or player
- Use RAII principles with smart pointers when appropriate

#### Error Handling
- Wrap media operations in try-catch blocks
- Log detailed error information for debugging

#### Performance
- Pre-allocate buffers for audio and video data
- Consider using a fixed-size thread pool for parallel decoding

#### Thread Safety
- Avoid calling player methods from multiple threads unnecessarily
- Use thread-safe containers for sharing data between threads


### Limitations
- **Supported Formats**: Limited to FFmpeg-supported formats/codecs
- **Resource Usage**: High-resolution video decoding can be resource-intensive
- **Synchronization**: Perfect A/V sync may be challenging with some media
- **DRM**: No support for DRM-protected content
