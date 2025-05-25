# Минимальная требуемая версия CMake
cmake_minimum_required(VERSION 3.10)

# Название проекта
project(ButtonTest)

# Указываем стандарт C++ (C++17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Путь к SFML (замените на ваш путь, если отличается)
set(SFML_INCLUDE_DIR "/opt/homebrew/Cellar/sfml@2/2.6.2_1/include")
set(SFML_LIB_DIR "/opt/homebrew/Cellar/sfml@2/2.6.2_1/lib")
set(SFML_DIR "/opt/homebrew/Cellar/sfml@2/2.6.2_1/lib/cmake/sfml")  # Apple Silicon

include_directories(${SFML_INCLUDE_DIR})
link_directories(${SFML_LIBRARY_DIR})

add_executable(ButtonTest main.cpp src/Button.cpp src/PlayButton.cpp src/PauseButton.cpp src/ProgressBar.cpp src/window/window.cpp)


# Поиск и подключение SFML
find_package(SFML 2.6 COMPONENTS graphics window system REQUIRED
             PATHS ${SFML_LIB_DIR} NO_DEFAULT_PATH)

# Затем линкуем библиотеки вручную
target_link_libraries(${PROJECT_NAME}
    sfml-graphics
    sfml-window
    sfml-audio
    sfml-system
)

# Подключаем SFML к проекту
target_link_libraries(ButtonTest sfml-graphics sfml-window sfml-system)

# Включаем директории для поиска заголовочных файлов
target_include_directories(ButtonTest PRIVATE include ${SFML_INCLUDE_DIR})

# Указываем путь к библиотекам SFML
target_link_directories(ButtonTest PRIVATE ${SFML_LIB_DIR})

find_package(Freetype REQUIRED)
include_directories(${FREETYPE_INCLUDE_DIRS})
target_link_libraries(ButtonTest ${FREETYPE_LIBRARIES})
