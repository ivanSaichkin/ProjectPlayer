# Media Player Project Documentation

## Architecture Overview
The project is a media player that utilizes FFmpeg libraries for audio/video decoding and SFML for video rendering and audio playback. The main components are:

- **Player**: Manages playback, pause, stop, and seek functions.
- **MediaFile**: Loads media files and provides access to metadata.
- **Decoder**: Base class for decoders (audio/video).
- **VideoDecoder**: Decodes video and converts frames to SFML format.
- **AudioDecoder**: Decodes audio and buffers data for SFML Sound.

## Key Classes and Methods

### 1. Class Player (Player.hpp/cpp)
**Role**: Central class managing playback.

**Methods**:
- `Load(const std::string& filename)`: Loads a media file and initializes decoders (audio/video). Resets the time offset (`timeOffset_`).
- `Play()`: Starts the playback thread (`PlaybackLoop`). Sets the initial time (`startTime_`) for synchronization.
- `TogglePause()`: Pauses/resumes playback. Updates `timeOffset_` for correct resumption.
- `Stop()`: Stops decoders and ends the playback thread.
- `Seek(int seconds)`: Moves the playback position. Resets decoder buffers (`Flush`).
- `Draw(sf::RenderWindow& window)`: Renders the current video frame via `VideoDecoder`.

**Fields**:
- `mediaFile_`: Loaded media file.
- `videoDecoder_, audioDecoder_`: Decoders for video and audio.
- `startTime_`: Start time for synchronization.
- `isRunning_, isPaused_`: Atomic state flags.

### 2. Class MediaFile (MediaFile.hpp/cpp)
**Role**: Works with media files through FFmpeg.

**Methods**:
- `Load(const std::string& filename)`: Opens the file and identifies audio/video streams. Initializes `AVFormatContext`.
- `GetVideoStreamIndex(), GetAudioStreamIndex()`: Return stream indices.
- `GetVideoTimeBase(), GetAudioTimeBase()`: Return time base for synchronization.

**Fields**:
- `formatContext_`: FFmpeg format context.
- `videoStreamIndex_, audioStreamIndex_`: Stream indices.

### 3. Class Decoder (Decoder.hpp)
**Role**: Base class for decoders.

**Methods**:
- `Start()`: Starts the decoding thread (virtual).
- `Stop()`: Stops the decoder and notifies condition variables.
- `Flush()`: Resets buffers and states (virtual).
- `SetPaused(bool paused)`: Manages pause state.
- `ProcessPacket(AVPacket* packet)`: Processes data packet (virtual).

**Fields**:
- `packetQueue_`: Queue of packets for decoding.
- `isRunning_, isPaused_, endOfStream_`: Decoder states.
- `packetCondition_`: Condition variable for thread synchronization.

### 4. Class VideoDecoder (VideoDecoder.hpp/cpp)
**Role**: Decodes video and renders through SFML.

**Methods**:
- `DecodeVideo()`: Main loop for decoding video from `packetQueue_`. Uses `swsContext` to convert frames to RGBA.
- `ProcessVideoFrame(AVFrame* frame)`: Synchronizes frame display time. Updates SFML texture (`texture_`).

**Fields**:
- `videoCodecContext_`: Video codec context.
- `swsContext_`: Scaling/conversion context from FFmpeg.
- `texture_, sprite_`: SFML objects for rendering.

### 5. Class AudioDecoder (AudioDecoder.hpp/cpp)
**Role**: Decodes audio and plays it through SFML.

**Methods**:
- `DecodeAudio()`: Main loop for audio decoding. Converts audio data to `int16_t` format for SFML.
- `ProcessAudioFrame(AVFrame* frame)`: Accumulates samples in `audioBuffer_`. Sends data to `sf::SoundBuffer` when threshold is reached.

**Fields**:
- `audioCodecContext_`: Audio codec context.
- `sound_, soundBuffer_`: SFML objects for playback.
- `audioBuffer_`: Buffer for accumulating samples.

## Interaction Diagram

### Initialization:
`Player::Load()` creates `MediaFile`, `VideoDecoder`, and `AudioDecoder`.

### Playback:
`Player::Play()` starts `PlaybackLoop` in a separate thread. `PlaybackLoop` reads packets from the file and distributes them to decoders.

### Decoding:
- **Video**: `VideoDecoder` decodes packets, converts frames, and updates the texture.
- **Audio**: `AudioDecoder` accumulates samples and periodically sends them to SFML.

### Synchronization:
- **Video**: Frames are displayed considering time (`startTime_`).
- **Audio**: Playback through SFML is automatically synchronized.

### Completion:
When the end of the file is reached, decoders receive a `SignalEndOfStream`. Remaining data in buffers is sent for playback.
