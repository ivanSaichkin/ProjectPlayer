cmake_minimum_required(VERSION 3.10)
# Название проекта
project(ButtonWindow)

set(CMAKE_CXX_STANDARD 17)

add_executable(ButtonWindow
/Users/andreypavlinich/ProjectPlayer-1/VideoPlayerFront/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/VideoPlayerBack/src/core/MediaPLayer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/VideoPlayerBack/src/core/AudioDecoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/VideoPlayerBack/src/core/ErrorHandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/VideoPlayerBack/src/core/MediaPlayerManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/VideoPlayerBack/src/core/VideoDecoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/VideoPlayerBack/src/playlist/Playlist.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/VideoPlayerBack/src/playlist/PlaylistManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/VideoPlayerBack/src/storage/ConfigManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/VideoPlayerBack/src/storage/FileManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/VideoPlayerBack/src/storage/HistoryManager.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/VideoPlayerFront/src/Button.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/VideoPlayerFront/src/PauseButton.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/VideoPlayerFront/src/PlayButton.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/VideoPlayerFront/src/ProgressBar.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/VideoPlayerFront/src/window/window.cpp)


# Указываем стандарт C++ (C++17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Путь к SFML (замените на ваш путь, если отличается)
# set(SFML_INCLUDE_DIR "/opt/homebrew/Cellar/sfml@2/SFML-2.5.1-macos-clang/include")
# set(SFML_LIB_DIR "/opt/homebrew/Cellar/sfml@2/SFML-2.5.1-macos-clang/lib")
# set(SFML_DIR "/opt/homebrew/Cellar/sfml@2/SFML-2.5.1-macos-clang/lib/cmake/sfml")  # Apple Silicon

include_directories(${SFML_INCLUDE_DIR})
link_directories(${SFML_LIBRARY_DIR})

# Поиск и подключение SFML
find_package(SFML 2.5.1 COMPONENTS graphics window system REQUIRED)

set(FFMPEG_DIR "/opt/homebrew")

include_directories(${FFMPEG_DIR}/include)
link_directories(${FFMPEG_DIR}/lib)

# Затем линкуем библиотеки вручную
target_link_libraries(ButtonWindow
    sfml-graphics
    sfml-window
    sfml-audio
    sfml-system
    avcodec avformat avutil swscale swresample
    pthread m z
)

# Подключаем SFML к проекту
target_link_libraries(ButtonWindow sfml-graphics sfml-window sfml-system)

# Включаем директории для поиска заголовочных файлов
target_include_directories(ButtonWindow PRIVATE include ${SFML_INCLUDE_DIR})

# Указываем путь к библиотекам SFML
target_link_directories(ButtonWindow PRIVATE ${SFML_LIB_DIR})

find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED libavcodec)
add_definitions(-DLIBAVCODEC_VERSION_MAJOR=${FFMPEG_VERSION_MAJOR})
