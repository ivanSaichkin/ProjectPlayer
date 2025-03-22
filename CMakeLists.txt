cmake_minimum_required(VERSION 3.10)
project(VideoPlayer)

set(CMAKE_CXX_STANDARD 17)
find_package(SFML 2.5 COMPONENTS graphics audio window system REQUIRED)

# Пути к FFmpeg (в WSL они уже установлены в системе)
include_directories(/usr/include) # Заголовочные файлы FFmpeg
link_directories(/usr/lib/x86_64-linux-gnu)

add_executable(VideoPlayer
    ${CMAKE_CURRENT_SOURCE_DIR}/Test/Test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/VideoPlayer/src/MediaFile.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/VideoPlayer/src/VideoDecoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/VideoPlayer/src/AudioDecoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/VideoPlayer/src/Player.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/VideoPlayer/src/Decoder.cpp
)


target_link_libraries(VideoPlayer
    sfml-graphics sfml-audio sfml-window sfml-system
    avcodec avformat avutil swscale swresample
    pthread m z
)
