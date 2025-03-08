# MediaFile

### MediaFile::Load Function

1. **Parameters and Return Type**
   - The function takes a filename as a string parameter and returns a boolean.

2. **Storing Filename**
   - It starts by storing the filename in the `filePath_` member variable.

3. **Opening the Media File**
   - `avformat_open_input` is called to open the media file:
     - It takes a pointer to `formatContext_` (which will be allocated by the function).
     - The filename is passed as a C-string.
     - The last two parameters are null, meaning no format is forced, and no options are passed.
     - If this fails (returns non-zero), it throws a runtime error.

4. **Reading Stream Information**
   - `avformat_find_stream_info` is called to read packets from the file to get stream information:
     - If this fails (returns negative), it throws a runtime error.

5. **Looping Through Streams**
   - The code then loops through all streams in the format context:
     - For each stream, it checks the codec type.
     - If it's a video stream, it stores the index in `videoStreamIndex_`.
     - If it's an audio stream, it stores the index in `audioStreamIndex_`.

6. **Checking for Video Stream**
   - After the loop, it checks if a video stream was found:
     - If not (i.e., `videoStreamIndex_` is still -1), it throws a runtime error.

7. **Returning Success**
   - If everything succeeds, the function returns true.

### Summary
This function is responsible for opening a media file, retrieving its format information, and identifying the video and audio streams within the file. It's using the FFmpeg library (evident from the `av*` function calls) to handle the media file operations.


# VideoDecoder
### VideoDecoder Class Overview

1. ### Inheritance
   - `VideoDecoder` inherits from a base `Decoder` class.

2. ### Constructor

1. **Parameters**
   - The constructor takes a `const` reference to a `MediaFile` object as its parameter.

2. **Initializer List**
   - The initializer list initializes `mediaFile_` with the passed `MediaFile` object and sets `videoCodecContext_` and `swsContext_` to `nullptr`.

3. **Initial State**
   - Inside the constructor:
     - `isPaused_` and `isRunning_` are set to `false`, indicating that the decoder is not paused or running initially.

4. **Retrieving Codec Parameters**
   - It retrieves the codec parameters from the `MediaFile` object:
     - It gets the format context from the `MediaFile`.
     - It accesses the codec parameters (`codecpar`) of the video stream.

5. **Finding the Decoder**
   - It finds the appropriate decoder for the codec:
     - `avcodec_find_decoder()` is called with the codec ID from the codec parameters.
     - If no codec is found, it throws a `runtime_error`.

6. **Allocating Video Codec Context**
   - It allocates and sets up the video codec context:
     - `avcodec_alloc_context3()` allocates the context.
     - `avcodec_parameters_to_context()` copies the parameters to the context.
     - `avcodec_open2()` opens the codec. If it fails, it throws a `runtime_error`.

7. **Setting Up Software Scaling Context**
   - It sets up the software scaling context (`swsContext_`):
     - This is used for converting between different pixel formats.
     - It's set up to convert from the input format to `AV_PIX_FMT_RGB32`.

8. **Creating SFML Texture**
   - Finally, it creates an SFML texture with the dimensions of the video:
     - This texture will be used to display the decoded video frames.

### Summary
This constructor essentially prepares the `VideoDecoder` object for decoding by setting up the necessary FFmpeg structures and SFML objects for video playback.

3. ### Destructor
   - Cleans up resources, including:
     - Freeing the video frame.
     - Freeing the scaling context.
     - Freeing the codec context.

4. ### Public Methods
   - `Start()`: Begins the video decoding process.
   - `Stop()`: Stops the video decoding.
   - `Draw(sf::RenderWindow& window)`: Renders the decoded video frame to the provided SFML window.
   - `TogglePause()`: Toggles the pause state of the decoder.

5. ### Private Method
   - `DecodeVideo()`: The main decoding loop that reads frames, decodes them, and prepares them for rendering.

6. ## Private Members
   - `videoCodecContext_`: Holds the FFmpeg codec context for video decoding.
   - `videoFrame_`: An FFmpeg frame object to hold decoded video data.
   - `swsContext_`: A software scaling context for format conversion.
   - `texture_`: An SFML texture to hold the decoded video frame.
   - `sprite_`: An SFML sprite used for drawing the texture.
   - `mediaFile_`: Stores the `MediaFile` object passed to the constructor.
