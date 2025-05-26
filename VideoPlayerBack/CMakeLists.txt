cmake_minimum_required(VERSION 3.10)
project(VideoPlayer)

set(CMAKE_CXX_STANDARD 17)
find_package(SFML 2.6 COMPONENTS graphics window system REQUIRED
             PATHS ${SFML_LIB_DIR} NO_DEFAULT_PATH)

# Пути к FFmpeg (в WSL они уже установлены в системе)
# include_directories(/usr/include) # Заголовочные файлы FFmpeg
# link_directories(/usr/lib/x86_64-linux-gnu)



find_package(nlohmann_json 3.11.2 REQUIRED)  # Проверьте актуальную версию
target_link_libraries(YourProject PRIVATE nlohmann_json::nlohmann_json)

set(FFMPEG_DIR "/usr/local/opt/ffmpeg")
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED
    libavcodec
    libavformat
    libavutil
    libswscale
)

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
