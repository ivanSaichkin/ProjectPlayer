cmake_minimum_required(VERSION 3.10)
project(VideoPlayer)

set(CMAKE_CXX_STANDARD 17)
find_package(SFML 2.5 COMPONENTS graphics audio window system REQUIRED)

# Пути к FFmpeg (в WSL они уже установлены в системе)
include_directories(/usr/include) # Заголовочные файлы FFmpeg
link_directories(/usr/lib/x86_64-linux-gnu)

add_executable(VideoPlayer
    ${CMAKE_CURRENT_SOURCE_DIR}/Test/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/API/MediaPlayer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/VideoDecoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AudioDecoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MediaDecoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ErrorHandler.cpp
)


target_link_libraries(VideoPlayer
    sfml-graphics sfml-audio sfml-window sfml-system
    avcodec avformat avutil swscale swresample
    pthread m z
)
