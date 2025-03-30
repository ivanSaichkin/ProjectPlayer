#include <iostream>

#include "../VideoPlayer/include/Player.hpp"
#include "../Window/include/Window.hpp"

int main(int argc, char* argv[]) {
    // Инициализация FFmpeg
    av_log_set_level(AV_LOG_QUIET);

    if (argc < 2) {
        std::cerr << "Использование: " << argv[0] << " <видеофайл>" << std::endl;
        return 1;
    }

    std::string filename = argv[1];

    // Создаем плеер
    Player player;

    try {
        // Загружаем видео
        player.Load(filename);

        // Получаем размер видео для создания окна соответствующего размера
        sf::Vector2i videoSize = player.GetVideoSize();

        // Если видео слишком большое, ограничиваем размер окна
        sf::VideoMode desktopMode = sf::VideoMode::getDesktopMode();
        int maxWidth = static_cast<int>(desktopMode.width * 0.9);
        int maxHeight = static_cast<int>(desktopMode.height * 0.9);

        int windowWidth = videoSize.x;
        int windowHeight = videoSize.y;

        // Если видео больше экрана, масштабируем окно
        if (windowWidth > maxWidth || windowHeight > maxHeight) {
            float scaleX = static_cast<float>(maxWidth) / windowWidth;
            float scaleY = static_cast<float>(maxHeight) / windowHeight;
            float scale = std::min(scaleX, scaleY);

            windowWidth = static_cast<int>(windowWidth * scale);
            windowHeight = static_cast<int>(windowHeight * scale);
        }

        // Минимальный размер окна
        windowWidth = std::max(windowWidth, 640);
        windowHeight = std::max(windowHeight, 480);

        // Создаем окно
        Window window(windowWidth, windowHeight, "Видеоплеер - " + filename);
        if (!window.Open()) {
            std::cerr << "Не удалось создать окно" << std::endl;
            return 1;
        }

        // Устанавливаем начальную громкость
        player.SetVolume(50.0f);

        // Устанавливаем обработчик изменения размера окна
        window.SetResizeCallback([&player](int width, int height) {
            // Здесь можно выполнить дополнительные действия при изменении размера окна
            std::cout << "Размер окна изменен на " << width << "x" << height << std::endl;
        });

        // Устанавливаем обработчик закрытия окна
        window.SetCloseCallback([&player]() {
            player.Stop();
        });

        // Начинаем воспроизведение
        player.Play();

        // Загружаем шрифт для отображения информации
        sf::Font font;
        bool fontLoaded = false;

        // Пробуем загрузить шрифт из нескольких возможных мест
        std::vector<std::string> fontPaths = {
            "arial.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/TTF/DejaVuSans.ttf",
            "C:\\Windows\\Fonts\\arial.ttf"
        };

        for (const auto& path : fontPaths) {
            if (font.loadFromFile(path)) {
                fontLoaded = true;
                break;
            }
        }

        // Если не удалось загрузить шрифт, создаем простой шрифт
        if (!fontLoaded) {
            std::cerr << "Не удалось загрузить шрифт. Информация о воспроизведении не будет отображаться." << std::endl;
        }

        // Создаем текст для отображения информации
        sf::Text timeText;
        if (fontLoaded) {
            timeText.setFont(font);
            timeText.setCharacterSize(20);
            timeText.setFillColor(sf::Color::White);
            timeText.setOutlineColor(sf::Color::Black);
            timeText.setOutlineThickness(1.0f);
            timeText.setPosition(10, 10);
        }

        // Главный цикл
        while (window.IsOpen()) {
            sf::Event event;
            while (window.PollEvent(event)) {
                // События обрабатываются в Window::PollEvent

                // Обрабатываем нажатия клавиш
                if (event.type == sf::Event::KeyPressed) {
                    switch (event.key.code) {
                        case sf::Keyboard::Space:
                            player.TogglePause();
                            break;
                        case sf::Keyboard::Escape:
                            window.Close();
                            break;
                        case sf::Keyboard::Left:
                            player.Seek(-10); // Перемотка на 10 секунд назад
                            break;
                        case sf::Keyboard::Right:
                            player.Seek(10);  // Перемотка на 10 секунд вперед
                            break;
                        case sf::Keyboard::Up:
                            player.SetVolume(std::min(100.0f, player.GetVolume() + 5.0f));
                            break;
                        case sf::Keyboard::Down:
                            player.SetVolume(std::max(0.0f, player.GetVolume() - 5.0f));
                            break;
                        case sf::Keyboard::F:
                            window.ToggleFullscreen();
                            break;
                        default:
                            break;
                    }
                }
            }

            // Очищаем окно
            window.Clear();

            // Отрисовываем видеокадр
            player.Draw(window);

            // Отображаем информацию о воспроизведении
            if (fontLoaded) {
                // Форматируем время в формате ММ:СС
                auto formatTime = [](double seconds) -> std::string {
                    int totalSeconds = static_cast<int>(seconds);
                    int minutes = totalSeconds / 60;
                    int secs = totalSeconds % 60;

                    std::string result;
                    if (minutes < 10) result += "0";
                    result += std::to_string(minutes) + ":";
                    if (secs < 10) result += "0";
                    result += std::to_string(secs);

                    return result;
                };

                std::string timeInfo = "Время: " +
                    formatTime(player.GetCurrentTime()) + " / " +
                    formatTime(player.GetDuration()) +
                    "   Громкость: " + std::to_string(static_cast<int>(player.GetVolume())) + "%";

                timeText.setString(timeInfo);

                // Создаем полупрозрачный фон для текста
                sf::RectangleShape textBackground;
                textBackground.setSize(sf::Vector2f(timeText.getLocalBounds().width + 20, 30));
                textBackground.setFillColor(sf::Color(0, 0, 0, 150));
                textBackground.setPosition(5, 5);

                window.GetRenderWindow().draw(textBackground);
                window.GetRenderWindow().draw(timeText);
            }

            // Отображаем содержимое окна
            window.Display();
        }

        // Останавливаем воспроизведение
        player.Stop();

    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
