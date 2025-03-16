```markdown
# Video Player Project Class Documentation

## 1. Class `MediaFile`
**Purpose**: Loading and managing media files, extracting stream information (video/audio).
**Dependencies**: FFmpeg (`libavformat`).

### Methods:
- **`load(const std::string& filename)`**:
  Loads the file and identifies streams.
  **Errors**:
  - Throws `MediaFileError` if the file is not found or the format is unsupported.

- **`getVideoStreamIndex()`**:
  Returns the index of the video stream.

- **`getAudioStreamIndex()`**:
  Returns the index of the audio stream.

- **`getVideoTimeBase()`**:
  Returns the time base (in seconds per PTS/DTS unit) for video synchronization.

- **`getAudioTimeBase()`**:
  Returns the time base (in seconds per PTS/DTS unit) for audio synchronization.

---

## 2. Class `VideoDecoder`
**Purpose**: Decoding the video stream and rendering frames via SFML.
**Dependencies**: FFmpeg (`libavcodec`, `libswscale`), SFML (`sf::Texture`).

### Methods:
- **`start()`**:
  Starts the video decoding thread.

- **`stop()`**:
  Stops the decoding process.

- **`togglePause()`**:
  Pauses/resumes decoding.

- **`draw(sf::RenderWindow& window)`**:
  Renders the current frame in the SFML window.

- **`flush()`**:
  Flushes the decoder buffers (used during seeking).

### Decoding Algorithm:
1. Read packets from `MediaFile`.
2. Convert pixel format (e.g., YUV -> RGB) using `sws_scale`.
3. Synchronize timing using PTS.

---

## 3. Class `AudioDecoder`
**Purpose**: Decoding the audio stream and playback via SFML.
**Dependencies**: FFmpeg (`libavcodec`), SFML (`sf::Sound`).

### Methods:
- **`start()`**:
  Starts the audio decoding thread.

- **`stop()`**:
  Stops audio playback.

- **`setVolume(float volume)`**:
  Sets the volume level (0–100).

- **`flush()`**:
  Flushes the decoder buffers.

### Decoding Algorithm:
1. Read audio packets.
2. Convert data to PCM format (16-bit).
3. Playback via `sf::SoundBuffer`.

---

## 4. Class `Player`
**Purpose**: Managing playback, synchronizing video and audio.

### Methods:
- **`load(const std::string& filename)`**:
  Loads the file and initializes the decoders.

- **`play()`**:
  Starts playback from the current position.

- **`pause()`**:
  Pauses/resumes playback.

- **`seek(double seconds)`**:
  Seeks to the specified time.

- **`getCurrentTime()`**:
  Returns the current playback position in seconds.

### Synchronization:
- Uses system time (`std::chrono`) to synchronize video and audio PTS.

---

## Error Handling
### Exceptions:
- All classes throw `std::runtime_error` with a description of the issue.

### Checks:
- File format correctness.
- Presence of video and audio streams.
- Availability of codecs.

---

## Dependencies
- **FFmpeg**: Video/audio decoding.
- **SFML 2.5+**: Graphics and audio.
```
