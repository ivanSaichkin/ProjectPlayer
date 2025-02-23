# VideoPlayer Constructor

The selected code is the constructor for the `VideoPlayer` class. Let's break it down:

## Breakdown of the Code

1. **Constructor Declaration**
   `VideoPlayer::VideoPlayer():` This is the constructor declaration for the `VideoPlayer` class.

2. **Initialization List**
   The constructor uses an initialization list to set initial values for various member variables:
   - `formatContext`, `videoCodecParameters`, `videoCodec`, `videoCodecContext`, `videoFrame`, `packet`, and `swsContext` are all initialized to `nullptr`. These are likely pointers to FFmpeg structures used for video decoding.
   - `videoStreamIndex` and `audioStreamIndex` are set to `-1`, indicating that no video or audio stream has been found yet.
   - `audioCodecParameters`, `audioCodec`, and `audioCodecContext` are also initialized to `nullptr`. These are likely used for audio decoding.
   - `volume` is set to `100`, probably representing 100% volume.
   - `isPlaying` is set to `false`, indicating that the video is not playing initially.
   - `currentFrame` is set to `0`, representing the starting frame.
   - `filePath` is initialized as an empty string.
   - `currentFileIndex` is set to `0`, likely used for playlist functionality.

3. **Empty Constructor Body**
   The constructor body is empty `{}`, as all initializations are done in the initialization list.

## Summary

This constructor sets up the initial state of a `VideoPlayer` object, preparing it for later use. It ensures that all pointers are null, indexes are invalid, and other variables have sensible default values. This is good practice to avoid undefined behavior and to have a known initial state for the object.


# VideoPlayer Destructor

The destructor for the `VideoPlayer` class is responsible for cleaning up and freeing resources that were allocated during the lifetime of the `VideoPlayer` object. Let's break it down:

## Breakdown of the Code

1. **Stopping Playback**
   `stop();`
   This calls the `stop()` method, which likely stops any ongoing playback and sets the `isPlaying` flag to false.

2. **Resource Cleanup**
   The following blocks check if certain pointers are not null, and if so, they free the associated resources:
   - `av_frame_free(&videoFrame);`
     Frees the allocated video frame.
   - `av_packet_free(&packet);`
     Frees the allocated packet used for reading frames.
   - `avcodec_free_context(&videoCodecContext);`
     Frees the video codec context.
   - `avcodec_free_context(&audioCodecContext);`
     Frees the audio codec context.
   - `avformat_close_input(&formatContext);`
     Closes the input format context and frees its resources.
   - `sws_freeContext(swsContext);`
     Frees the software scaling context used for video frame conversion.

## Importance of Cleanup Operations

These cleanup operations are crucial for preventing memory leaks and ensuring that all resources allocated by the FFmpeg library (which this code seems to be using for video and audio processing) are properly released when the `VideoPlayer` object is destroyed.

The use of if-statements before freeing each resource is a good practice, as it prevents attempting to free null pointers, which could lead to undefined behavior or crashes.

# VideoPlayer Load Function

The `load` function in the `VideoPlayer` class is responsible for loading a video file and setting up the necessary components for video and audio playback. Let's break it down:

## Breakdown of the Code

1. **Opening the Video File**
   The function takes a filename as input and attempts to open the file using `avformat_open_input`.

2. **Finding Stream Information**
   It then tries to find stream information using `avformat_find_stream_info`.

3. **Identifying Video and Audio Streams**
   The code iterates through all streams in the file to identify video and audio streams, setting `videoStreamIndex` and `audioStreamIndex` accordingly.

4. **Checking for Video Stream**
   If no video stream is found, the function returns `false`.

5. **Setting Up the Video Stream**
   For the video stream:
   - It retrieves codec parameters and finds the appropriate decoder.
   - Allocates and sets up the codec context.
   - Opens the video codec.
   - Allocates frame and packet structures.
   - Sets up a scaling context (`swsContext`) for converting between pixel formats.
   - Creates an SFML texture and sprite for rendering.

6. **Setting Up the Audio Stream**
   If an audio stream is found:
   - It follows a similar process to set up the audio codec and context.
   - Initializes an SFML sound buffer and sound object for audio playback.

7. **Return Value**
   The function returns `true` if everything is set up successfully.

## Summary

This code uses the FFmpeg library (`libav*`) for handling video and audio decoding, and SFML for rendering and audio output. It's designed to handle both video and audio components of a media file, setting up the necessary structures and contexts for decoding and playback.

### Bug Note

There's a small bug in the code: `videoStreamIndex` is mistakenly set to `1` instead of `i` in the stream type checking loop. This should be corrected to `videoStreamIndex = i;`.


# Video Decoding Process

The `decodeVideo()` function of the `VideoPlayer` class is responsible for decoding video frames and updating the texture for rendering.

## Breakdown of the Code

1. **Continuous Decoding Loop**
   The function runs in a loop while `isPlaying` is true, which allows continuous video decoding.

2. **Reading Video Frames**
   The `av_read_frame(formatContext, packet)` function reads the next frame from the video file. If successful (returns >= 0), it continues processing.

3. **Checking Video Stream**
   It checks if the packet belongs to the video stream by comparing `packet->stream_index` with `videoStreamIndex`.

4. **Sending Packet to Decoder**
   `avcodec_send_packet(videoCodecContext, packet)` sends the packet to the decoder.

5. **Receiving Decoded Frames**
   If the packet is successfully sent, it enters another loop to receive decoded frames using `avcodec_receive_frame(videoCodecContext, videoFrame)`.

6. **Processing Decoded Frames**
   For each decoded frame:
   - It locks a mutex to ensure thread-safety.
   - Allocates memory for the pixel data.
   - Sets up the destination buffer for the frame conversion.
   - Uses `sws_scale` to convert the frame from its native format to RGB32.
   - Updates the SFML texture with the converted pixel data.
   - Frees the allocated pixel memory.
   - Increments the `currentFrame` counter.
   - Introduces a delay to maintain the correct frame rate.

7. **Freeing Resources**
   After processing, it unreferences the packet with `av_packet_unref(packet)` to free associated resources.

8. **Handling End of Stream**
   If `av_read_frame` fails (end of file or error), it sets `isPlaying` to false, ending the decoding loop.

This function runs in a separate thread to allow for smooth playback alongside audio decoding and user interface operations, ensuring an enjoyable viewing experience.

# Audio Decoding Process

The `decodeAudio` function is called when the audio stream is available and the player is playing.

## Breakdown of the Code

1. **Continuous Frame Reading**
   Inside the `decodeAudio` function, a while loop is used to continuously read frames from the audio stream until the player is stopped.

2. **Reading Audio Frames**
   The `av_read_frame` function is used to read the next frame from the input file. If the frame belongs to the audio stream (determined by comparing the `stream_index` with `audioStreamIndex`), the frame is sent to the audio codec using `avcodec_send_packet`.

3. **Receiving Decoded Frames**
   If the frame is successfully sent to the audio codec, another while loop is used to receive decoded audio frames from the codec using `avcodec_receive_frame`.

4. **Extracting Samples**
   For each received audio frame, the samples are extracted and stored in a `std::vector<int16_t>` named `samples`. The samples are obtained by converting the raw data from the audio frame to `int16_t`.

5. **Loading Samples into Sound Buffer**
   The `soundBuffer` object is then loaded with the extracted samples using the `loadFromSamples` function. The number of channels, sample rate, and the size of the samples are passed as parameters.

6. **Playing the Sound**
   Finally, the sound object is played using the `play` function.

This process continues until there are no more frames to read from the audio stream or the player is stopped. If the end of the audio stream is reached, the `isPlaying` flag is set to `false`, and the decoding process stops.
