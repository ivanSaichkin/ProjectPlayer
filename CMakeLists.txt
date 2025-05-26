# cmake_minimum_required(VERSION 3.10)
# # Название проекта
# project(ButtonWindow)

# set(CMAKE_CXX_STANDARD 17)

# add_executable(ButtonWindow
# /Users/andreypavlinich/ProjectPlayer-1/VideoPlayerFront/main.cpp
#     ${CMAKE_CURRENT_SOURCE_DIR}/VideoPlayerBack/src/core/MediaPLayer.cpp
#     ${CMAKE_CURRENT_SOURCE_DIR}/VideoPlayerBack/src/core/AudioDecoder.cpp
#     ${CMAKE_CURRENT_SOURCE_DIR}/VideoPlayerBack/src/core/ErrorHandler.cpp
#     ${CMAKE_CURRENT_SOURCE_DIR}/VideoPlayerBack/src/core/MediaPlayerManager.cpp
#     ${CMAKE_CURRENT_SOURCE_DIR}/VideoPlayerBack/src/core/VideoDecoder.cpp
#     ${CMAKE_CURRENT_SOURCE_DIR}/VideoPlayerBack/src/playlist/Playlist.cpp
#     ${CMAKE_CURRENT_SOURCE_DIR}/VideoPlayerBack/src/playlist/PlaylistManager.cpp
#     ${CMAKE_CURRENT_SOURCE_DIR}/VideoPlayerBack/src/storage/ConfigManager.cpp
#     ${CMAKE_CURRENT_SOURCE_DIR}/VideoPlayerBack/src/storage/FileManager.cpp
#     ${CMAKE_CURRENT_SOURCE_DIR}/VideoPlayerBack/src/storage/HistoryManager.cpp

#     ${CMAKE_CURRENT_SOURCE_DIR}/VideoPlayerFront/src/Button.cpp
#     ${CMAKE_CURRENT_SOURCE_DIR}/VideoPlayerFront/src/PauseButton.cpp
#     ${CMAKE_CURRENT_SOURCE_DIR}/VideoPlayerFront/src/PlayButton.cpp
#     ${CMAKE_CURRENT_SOURCE_DIR}/VideoPlayerFront/src/ProgressBar.cpp
#     ${CMAKE_CURRENT_SOURCE_DIR}/VideoPlayerFront/src/window/window.cpp)


# # Указываем стандарт C++ (C++17)
# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)

# # Путь к SFML (замените на ваш путь, если отличается)
# set(SFML_INCLUDE_DIR "/opt/homebrew/Cellar/sfml@2/SFML-2.5.1-macos-clang/include")
# set(SFML_LIB_DIR "/opt/homebrew/Cellar/sfml@2/SFML-2.5.1-macos-clang/lib")
# set(SFML_DIR "/opt/homebrew/Cellar/sfml@2/SFML-2.5.1-macos-clang/lib/cmake/sfml")  # Apple Silicon

# include_directories(${SFML_INCLUDE_DIR})
# link_directories(${SFML_LIBRARY_DIR})

# # Поиск и подключение SFML
# find_package(SFML 2.5.1 COMPONENTS graphics window system REQUIRED)

# set(FFMPEG_INCLUDE_DIR "/opt/homebrew/Cellar/ffmpeg@6/6.1.2_9/include")
# set(FFMPEG_LIB_DIR "/opt/homebrew/Cellar/ffmpeg@6/6.1.2_9/lib")

# include_directories(${FFMPEG_INCLUDE_DIR})
# link_directories(${FFMPEG_LIB_DIR})

# # Затем линкуем библиотеки вручную
# target_link_libraries(ButtonWindow
#     sfml-graphics
#     sfml-window
#     sfml-audio
#     sfml-system
#     avcodec avformat avutil swscale swresample
#     pthread m z
# )

# # Подключаем SFML к проекту
# target_link_libraries(ButtonWindow sfml-graphics sfml-window sfml-system)

# # Включаем директории для поиска заголовочных файлов
# target_include_directories(ButtonWindow PRIVATE include ${SFML_INCLUDE_DIR})

# # Указываем путь к библиотекам SFML
# target_link_directories(ButtonWindow PRIVATE ${SFML_LIB_DIR})

# find_package(PkgConfig REQUIRED)
# pkg_check_modules(FFMPEG REQUIRED libavcodec)
# add_definitions(-DLIBAVCODEC_VERSION_MAJOR=${FFMPEG_VERSION_MAJOR})

cmake_minimum_required(VERSION 3.10)
project(ButtonWindow)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Добавляем исполняемый файл
add_executable(ButtonWindow
    /Users/andreypavlinich/ProjectPlayer-1/VideoPlayerFront/test.cpp

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
    # ${CMAKE_CURRENT_SOURCE_DIR}/VideoPlayerFront/src/Button.cpp
    # ${CMAKE_CURRENT_SOURCE_DIR}/VideoPlayerFront/src/PauseButton.cpp
    # ${CMAKE_CURRENT_SOURCE_DIR}/VideoPlayerFront/src/PlayButton.cpp
    # ${CMAKE_CURRENT_SOURCE_DIR}/VideoPlayerFront/src/ProgressBar.cpp
    # ${CMAKE_CURRENT_SOURCE_DIR}/VideoPlayerFront/src/window/window.cpp
)

# Настройка SFML
set(SFML_DIR "/opt/homebrew/Cellar/sfml@2/2.6.2_1/lib/cmake/sfml")
find_package(SFML 2.6.1 COMPONENTS graphics window system audio REQUIRED)

# Настройка FFmpeg
set(FFMPEG_ROOT "/opt/homebrew/opt/ffmpeg@6")
include_directories(${FFMPEG_ROOT}/include)
link_directories(${FFMPEG_ROOT}/lib)

# Установка флагов для FFmpeg (убираем дублирование версии)
add_definitions(-Wno-deprecated-declarations)  # Отключаем предупреждения о deprecated

# Настройка JSON библиотеки
find_package(nlohmann_json 3.9.1 REQUIRED)

# Подключение зависимостей
target_include_directories(ButtonWindow PRIVATE
    ${SFML_INCLUDE_DIR}
    ${FFMPEG_ROOT}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include  # Добавляем путь к вашим заголовкам
)

target_link_directories(ButtonWindow PRIVATE
    ${SFML_LIB_DIR}
    ${FFMPEG_ROOT}/lib
)

target_link_libraries(ButtonWindow
    sfml-graphics
    sfml-window
    sfml-system
    sfml-audio
    avcodec
    avformat
    avutil
    swscale
    swresample
    pthread
    z
    nlohmann_json::nlohmann_json  # Подключаем JSON библиотеку
)

find_package(OpenAL REQUIRED)
