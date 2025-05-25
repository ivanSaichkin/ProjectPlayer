# Минимальная требуемая версия CMake
cmake_minimum_required(VERSION 3.10)

# Название проекта
project(ButtonExample)

# Указываем стандарт C++ (C++17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Путь к SFML (замените на ваш путь, если отличается)
set(SFML_INCLUDE_DIR "/opt/homebrew/Cellar/sfml@2/2.6.2_1/include")
set(SFML_LIB_DIR "/opt/homebrew/Cellar/sfml@2/2.6.2_1/lib")

# Поиск и подключение SFML
find_package(SFML 2.6 COMPONENTS graphics window system REQUIRED
             PATHS ${SFML_LIB_DIR} NO_DEFAULT_PATH)

# Добавляем исполняемый файл
add_executable(ButtonExample main.cpp src/Button.cpp src/PlayButton.cpp src/PauseButton.cpp src/ProgressBar.cpp)

# Подключаем SFML к проекту
target_link_libraries(ButtonExample sfml-graphics sfml-window sfml-system)

# Включаем директории для поиска заголовочных файлов
target_include_directories(ButtonExample PRIVATE include ${SFML_INCLUDE_DIR})

# Указываем путь к библиотекам SFML
target_link_directories(ButtonExample PRIVATE ${SFML_LIB_DIR})

find_package(Freetype REQUIRED)
include_directories(${FREETYPE_INCLUDE_DIRS})
target_link_libraries(ButtonExample ${FREETYPE_LIBRARIES})
