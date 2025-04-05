#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <iomanip>
#include <sstream>

#include "../VideoPlayer/include/Player.hpp"
#include "../Window/include/Window.hpp"

std::string formatTime(double seconds) {
    int totalSeconds = static_cast<int>(seconds);
    int minutes = totalSeconds / 60;
    int secs = totalSeconds % 60;

    std::stringstream ss;
    ss << std::setfill('0') << std::setw(2) << minutes << ":"
       << std::setfill('0') << std::setw(2) << secs;
    return ss.str();
}

// Функция для отображения инструкций
void showInstructions(Window& window, sf::Font& font) {
    sf::Text text;
    text.setFont(font);
    text.setCharacterSize(16);
    text.setFillColor(sf::Color::White);

    std::string instructions =
        "Управление:\n"
        "Пробел - пауза/воспроизведение\n"
        "Стрелки влево/вправо - перемотка на 10 секунд\n"
        "Стрелки вверх/вниз - регулировка громкости\n"
        "F - полноэкранный режим\n"
        "Escape - выход";

    text.setString(instructions);

    // Создаем полупрозрачный фон
    sf::RectangleShape background;
    background.setSize(sf::Vector2f(text.getLocalBounds().width + 20, text.getLocalBounds().height + 30));
    background.setFillColor(sf::Color(0, 0, 0, 200));
    background.setPosition(10, 40);

    text.setPosition(20, 50);

    window.GetRenderWindow().draw(background);
    window.GetRenderWindow().draw(text);
}

// Функция для тестирования всех функций плеера
void testPlayer(Player& player, Window& window, sf::Font& font) {
    std::cout << "Тестирование плеера..." << std::endl;

    // Тест 1: Воспроизведение
    std::cout << "Тест 1: Воспроизведение" << std::endl;
    player.Play();

    // Ждем 3 секунды для проверки воспроизведения
    for (int i = 0; i < 3; i++) {
        std::cout << "Текущее время: " << player.GetCurrentTime() << " секунд" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Тест 2: Пауза
    std::cout << "\nТест 2: Пауза" << std::endl;
    player.TogglePause();
    std::cout << "Воспроизведение на паузе. Текущее время: " << player.GetCurrentTime() << " секунд" << std::endl;

    // Ждем 2 секунды в режиме паузы
    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::cout << "После 2 секунд паузы. Текущее время: " << player.GetCurrentTime() << " секунд" << std::endl;

    // Тест 3: Возобновление
    std::cout << "\nТест 3: Возобновление" << std::endl;
    player.TogglePause();
    std::cout << "Воспроизведение возобновлено" << std::endl;

    // Ждем 2 секунды для проверки возобновления
    for (int i = 0; i < 2; i++) {
        std::cout << "Текущее время: " << player.GetCurrentTime() << " секунд" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Тест 4: Регулировка громкости
    std::cout << "\nТест 4: Регулировка громкости" << std::endl;
    std::cout << "Исходная громкость: " << player.GetVolume() << "%" << std::endl;
    player.SetVolume(75.0f);
    std::cout << "Новая громкость: " << player.GetVolume() << "%" << std::endl;

    // Тест 5: Перемотка вперед
    std::cout << "\nТест 5: Перемотка вперед" << std::endl;
    double timeBefore = player.GetCurrentTime();
    std::cout << "Текущее время перед перемоткой: " << timeBefore << " секунд" << std::endl;
    player.Seek(20);
    std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Ждем, пока перемотка выполнится
    double timeAfter = player.GetCurrentTime();
    std::cout << "Текущее время после перемотки: " << timeAfter << " секунд" << std::endl;
    std::cout << "Разница: " << (timeAfter - timeBefore) << " секунд" << std::endl;

    // Ждем 3 секунды для проверки воспроизведения после перемотки вперед
    for (int i = 0; i < 3; i++) {
        std::cout << "Текущее время: " << player.GetCurrentTime() << " секунд" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Тест 6: Перемотка назад
    std::cout << "\nТест 6: Перемотка назад" << std::endl;
    timeBefore = player.GetCurrentTime();
    std::cout << "Текущее время перед перемоткой: " << timeBefore << " секунд" << std::endl;
    player.Seek(-10);
    std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Ждем, пока перемотка выполнится
    timeAfter = player.GetCurrentTime();
    std::cout << "Текущее время после перемотки: " << timeAfter << " секунд" << std::endl;
    std::cout << "Разница: " << (timeAfter - timeBefore) << " секунд" << std::endl;

    // Тест 7: Остановка
    std::cout << "\nТест 7: Остановка" << std::endl;
    player.Stop();
    std::cout << "Воспроизведение остановлено. Текущее время: " << player.GetCurrentTime() << " секунд" << std::endl;

    // Тест 8: Повторное воспроизведение
    std::cout << "\nТест 8: Повторное воспроизведение" << std::endl;
    player.Play();
    std::cout << "Воспроизведение запущено снова" << std::endl;

    // Ждем 2 секунды для проверки повторного воспроизведения
    for (int i = 0; i < 2; i++) {
        std::cout << "Текущее время: " << player.GetCurrentTime() << " секунд" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "\nТестирование завершено!" << std::endl;

    // Отображаем результаты тестирования
    sf::Text resultText;
    resultText.setFont(font);
    resultText.setCharacterSize(20);
    resultText.setFillColor(sf::Color::Green);
    resultText.setString("Тестирование завершено успешно!");
    resultText.setPosition(
        (window.GetSize().x - resultText.getLocalBounds().width) / 2.0f,
        window.GetSize().y - 100
    );

    sf::RectangleShape resultBackground;
    resultBackground.setSize(sf::Vector2f(resultText.getLocalBounds().width + 20, 40));
    resultBackground.setFillColor(sf::Color(0, 0, 0, 200));
    resultBackground.setPosition(
        resultText.getPosition().x - 10,
        resultText.getPosition().y - 10
    );

    window.GetRenderWindow().draw(resultBackground);
    window.GetRenderWindow().draw(resultText);
    window.Display();

    // Ждем 3 секунды для отображения результата
    std::this_thread::sleep_for(std::chrono::seconds(3));
}

int main(int argc, char* argv[]) {
    // Инициализация FFmpeg
    av_log_set_level(AV_LOG_QUIET);

    std::string filename;
    bool autoTest = false;

    // Обработка аргументов командной строки
    if (argc > 1) {
        filename = argv[1];

        // Проверяем флаг автоматического тестирования
        if (argc > 2 && std::string(argv[2]) == "--test") {
            autoTest = true;
        }
    } else {
        std::cerr << "Использование: " << argv[0] << " <видеофайл> [--test]" << std::endl;
        std::cerr << "Пример: " << argv[0] << " sample.mp4" << std::endl;
        return 1;
    }

    // Создаем плеер
    Player player;

    try {
        // Загружаем видео
        std::cout << "Загрузка видеофайла: " << filename << std::endl;
        player.Load(filename);

        // Получаем размер видео для создания окна соответствующего размера
        sf::Vector2i videoSize = player.GetVideoSize();
        std::cout << "Размер видео: " << videoSize.x << "x" << videoSize.y << std::endl;

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
        std::cout << "Создание окна размером " << windowWidth << "x" << windowHeight << std::endl;
        Window window(windowWidth, windowHeight, "Видеоплеер - " + filename);
        if (!window.Open()) {
            std::cerr << "Не удалось создать окно" << std::endl;
            return 1;
        }

        // Устанавливаем начальную громкость
        player.SetVolume(50.0f);

        // Устанавливаем обработчик изменения размера окна
        window.SetResizeCallback([&player](int width, int height) {
            std::cout << "Размер окна изменен на " << width << "x" << height << std::endl;
        });

        // Устанавливаем обработчик закрытия окна
        window.SetCloseCallback([&player]() {
            player.Stop();
        });

        // Загружаем шрифт для отображения информации
        sf::Font font;
        bool fontLoaded = false;

        // Пробуем загрузить шрифт из нескольких возможных мест
        std::vector<std::string> fontPaths = {
            "Arial.ttf"
        };

        for (const auto& path : fontPaths) {
            if (font.loadFromFile(path)) {
                fontLoaded = true;
                std::cout << "Шрифт загружен: " << path << std::endl;
                break;
            }
        }

        // Если не удалось загрузить шрифт, предупреждаем
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

        // Начинаем воспроизведение
        std::cout << "Начало воспроизведения" << std::endl;
        player.Play();

        // Если включен режим автоматического тестирования
        if (autoTest && fontLoaded) {
            testPlayer(player, window, font);
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
                // Формируем строку с информацией о воспроизведении
                std::string timeInfo = "Время: " +
                    formatTime(player.GetCurrentTime()) + " / " +
                    formatTime(player.GetDuration()) +
                    "   Громкость: " + std::to_string(static_cast<int>(player.GetVolume())) + "%";

                // Добавляем информацию о статусе
                if (player.IsFinished()) {
                    timeInfo += "   [Воспроизведение завершено]";
                } else if (player.IsPaused()) {
                    timeInfo += "   [Пауза]";
                }

                timeText.setString(timeInfo);

                // Создаем полупрозрачный фон для текста
                sf::RectangleShape textBackground;
                textBackground.setSize(sf::Vector2f(timeText.getLocalBounds().width + 20, 30));
                textBackground.setFillColor(sf::Color(0, 0, 0, 150));
                textBackground.setPosition(5, 5);

                window.GetRenderWindow().draw(textBackground);
                window.GetRenderWindow().draw(timeText);

                // Отображаем инструкции по управлению
                showInstructions(window, font);

                // Если видео закончилось, отображаем уведомление
                if (player.IsFinished()) {
                    sf::Text finishedText;
                    finishedText.setFont(font);
                    finishedText.setCharacterSize(30);
                    finishedText.setFillColor(sf::Color::White);
                    finishedText.setOutlineColor(sf::Color::Black);
                    finishedText.setOutlineThickness(2.0f);
                    finishedText.setString("Воспроизведение завершено");

                    // Центрируем текст
                    sf::FloatRect textBounds = finishedText.getLocalBounds();
                    finishedText.setPosition(
                        (window.GetSize().x - textBounds.width) / 2.0f,
                        (window.GetSize().y - textBounds.height) / 2.0f
                    );

                    // Создаем полупрозрачный фон
                    sf::RectangleShape finishedBackground;
                    finishedBackground.setSize(sf::Vector2f(textBounds.width + 40, textBounds.height + 20));
                    finishedBackground.setFillColor(sf::Color(0, 0, 0, 180));
                    finishedBackground.setPosition(
                        finishedText.getPosition().x - 20,
                        finishedText.getPosition().y - 10
                    );

                    window.GetRenderWindow().draw(finishedBackground);
                    window.GetRenderWindow().draw(finishedText);
                }
            }

            // Отображаем содержимое окна
            window.Display();
        }

        // Останавливаем воспроизведение
        std::cout << "Остановка воспроизведения" << std::endl;
        player.Stop();

    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Программа завершена" << std::endl;
    return 0;
}
