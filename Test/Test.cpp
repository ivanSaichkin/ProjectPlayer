#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <iomanip>
#include <sstream>

#include "../VideoPlayer/include/Player.hpp"
#include "../Window/include/Window.hpp"

// Функция для форматирования времени
std::string formatTime(double seconds) {
    int totalSeconds = static_cast<int>(seconds);
    int minutes = totalSeconds / 60;
    int secs = totalSeconds % 60;

    std::stringstream ss;
    ss << std::setfill('0') << std::setw(2) << minutes << ":"
       << std::setfill('0') << std::setw(2) << secs;
    return ss.str();
}

// Тестирование всех функций плеера
void testPlayer(Player& player, const std::string& filename) {
    std::cout << "\n==== НАЧАЛО ТЕСТИРОВАНИЯ ПЛЕЕРА ====\n" << std::endl;

    // Тест 1: Загрузка файла
    std::cout << "Тест 1: Загрузка файла" << std::endl;
    std::cout << "Загрузка: " << filename << std::endl;
    player.Load(filename);
    std::cout << "Длительность видео: " << player.GetDuration() << " секунд" << std::endl;
    std::cout << "Текущее время после загрузки: " << player.GetCurrentTime() << " секунд" << std::endl;
    std::cout << "Размер видео: " << player.GetVideoSize().x << "x" << player.GetVideoSize().y << std::endl;
    std::cout << "Тест 1 пройден: " << (player.GetCurrentTime() < 1.0 ? "УСПЕШНО" : "ОШИБКА") << "\n" << std::endl;

    // Тест 2: Воспроизведение
    std::cout << "Тест 2: Воспроизведение" << std::endl;
    player.Play();
    std::cout << "Началось воспроизведение" << std::endl;

    // Ждем 3 секунды для проверки воспроизведения
    for (int i = 0; i < 3; i++) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "Текущее время: " << player.GetCurrentTime() << " секунд" << std::endl;
    }

    double timeAfterPlay = player.GetCurrentTime();
    std::cout << "Тест 2 пройден: " << (timeAfterPlay > 1.0 ? "УСПЕШНО" : "ОШИБКА") << "\n" << std::endl;

    // Тест 3: Пауза
    std::cout << "Тест 3: Пауза" << std::endl;
    player.TogglePause();
    double timeBeforePause = player.GetCurrentTime();
    std::cout << "Воспроизведение на паузе. Текущее время: " << timeBeforePause << " секунд" << std::endl;

    // Ждем 2 секунды в режиме паузы
    std::this_thread::sleep_for(std::chrono::seconds(2));
    double timeAfterPause = player.GetCurrentTime();
    std::cout << "После 2 секунд паузы. Текущее время: " << timeAfterPause << " секунд" << std::endl;

    std::cout << "Тест 3 пройден: " << (std::abs(timeAfterPause - timeBeforePause) < 0.1 ? "УСПЕШНО" : "ОШИБКА") << "\n" << std::endl;

    // Тест 4: Возобновление
    std::cout << "Тест 4: Возобновление" << std::endl;
    player.TogglePause();
    std::cout << "Воспроизведение возобновлено" << std::endl;

    // Ждем 2 секунды для проверки возобновления
    for (int i = 0; i < 2; i++) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "Текущее время: " << player.GetCurrentTime() << " секунд" << std::endl;
    }

    double timeAfterResume = player.GetCurrentTime();
    std::cout << "Тест 4 пройден: " << (timeAfterResume > timeAfterPause ? "УСПЕШНО" : "ОШИБКА") << "\n" << std::endl;

    // Тест 5: Регулировка громкости
    std::cout << "Тест 5: Регулировка громкости" << std::endl;
    float initialVolume = player.GetVolume();
    std::cout << "Исходная громкость: " << initialVolume << "%" << std::endl;

    player.SetVolume(75.0f);
    float newVolume = player.GetVolume();
    std::cout << "Новая громкость: " << newVolume << "%" << std::endl;

    std::cout << "Тест 5 пройден: " << (std::abs(newVolume - 75.0f) < 0.1 ? "УСПЕШНО" : "ОШИБКА") << "\n" << std::endl;

    // Тест 6: Перемотка вперед
    std::cout << "Тест 6: Перемотка вперед" << std::endl;
    double timeBefore = player.GetCurrentTime();
    std::cout << "Текущее время перед перемоткой: " << timeBefore << " секунд" << std::endl;

    player.Seek(10);
    std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Ждем, пока перемотка выполнится

    double timeAfter = player.GetCurrentTime();
    std::cout << "Текущее время после перемотки: " << timeAfter << " секунд" << std::endl;
    std::cout << "Разница: " << (timeAfter - timeBefore) << " секунд" << std::endl;

    std::cout << "Тест 6 пройден: " << (timeAfter > timeBefore + 8.0 ? "УСПЕШНО" : "ОШИБКА") << "\n" << std::endl;

    // Ждем 2 секунды для проверки воспроизведения после перемотки вперед
    for (int i = 0; i < 2; i++) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "Текущее время: " << player.GetCurrentTime() << " секунд" << std::endl;
    }

    // Тест 7: Перемотка назад
    std::cout << "Тест 7: Перемотка назад" << std::endl;
    timeBefore = player.GetCurrentTime();
    std::cout << "Текущее время перед перемоткой: " << timeBefore << " секунд" << std::endl;

    player.Seek(-5);
    std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Ждем, пока перемотка выполнится

    timeAfter = player.GetCurrentTime();
    std::cout << "Текущее время после перемотки: " << timeAfter << " секунд" << std::endl;
    std::cout << "Разница: " << (timeAfter - timeBefore) << " секунд" << std::endl;

    std::cout << "Тест 7 пройден: " << (timeAfter < timeBefore - 3.0 ? "УСПЕШНО" : "ОШИБКА") << "\n" << std::endl;

    // Тест 8: Остановка
    std::cout << "Тест 8: Остановка" << std::endl;
    player.Stop();
    std::cout << "Воспроизведение остановлено. Текущее время: " << player.GetCurrentTime() << " секунд" << std::endl;

    std::cout << "Тест 8 пройден: " << (player.GetCurrentTime() < 1.0 ? "УСПЕШНО" : "ОШИБКА") << "\n" << std::endl;

    // Тест 9: Повторное воспроизведение
    std::cout << "Тест 9: Повторное воспроизведение" << std::endl;
    player.Play();
    std::cout << "Воспроизведение запущено снова" << std::endl;

    // Ждем 2 секунды для проверки повторного воспроизведения
    for (int i = 0; i < 2; i++) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "Текущее время: " << player.GetCurrentTime() << " секунд" << std::endl;
    }

    double finalTime = player.GetCurrentTime();
    std::cout << "Тест 9 пройден: " << (finalTime > 1.0 ? "УСПЕШНО" : "ОШИБКА") << "\n" << std::endl;

    std::cout << "==== ТЕСТИРОВАНИЕ ЗАВЕРШЕНО ====\n" << std::endl;
}

// Основная функция тестирования с графическим интерфейсом
void runGraphicalTest(const std::string& filename) {
    // Инициализация FFmpeg
    av_log_set_level(AV_LOG_QUIET);

    // Создаем плеер
    Player player;

    try {
        // Загружаем видео
        std::cout << "Загрузка видеофайла: " << filename << std::endl;
        player.Load(filename);

        // Получаем размер видео для создания окна соответствующего размера
        sf::Vector2i videoSize = player.GetVideoSize();
        std::cout << "Размер видео: " << videoSize.x << "x" << videoSize.y << std::endl;

        // Определяем размер окна
        int windowWidth = std::max(videoSize.x, 640);
        int windowHeight = std::max(videoSize.y, 480);

        // Создаем окно
        Window window(windowWidth, windowHeight, "Тестирование видеоплеера - " + filename);
        if (!window.Open()) {
            std::cerr << "Не удалось создать окно" << std::endl;
            return;
        }

        // Устанавливаем громкость
        player.SetVolume(50.0f);

        // Загружаем шрифт
        sf::Font font;
        bool fontLoaded = font.loadFromFile("Arial.ttf");

        if (!fontLoaded) {
            std::cerr << "Не удалось загрузить шрифт" << std::endl;
        }

        // Создаем текст для отображения информации
        sf::Text infoText;
        if (fontLoaded) {
            infoText.setFont(font);
            infoText.setCharacterSize(18);
            infoText.setFillColor(sf::Color::White);
            infoText.setPosition(10, 10);
        }

        // Создаем текст для инструкций
        sf::Text instructionsText;
        if (fontLoaded) {
            instructionsText.setFont(font);
            instructionsText.setCharacterSize(16);
            instructionsText.setFillColor(sf::Color::White);
            instructionsText.setPosition(10, window.GetSize().y - 150);

            instructionsText.setString(
                "ТЕСТОВЫЙ РЕЖИМ\n"
                "Space: Пауза/Воспроизведение\n"
                "Left/Right: Перемотка -/+ 10 сек\n"
                "Up/Down: Громкость +/-\n"
                "F: Полный экран\n"
                "Esc: Выход"
            );
        }

        // Начинаем воспроизведение
        player.Play();

        // Главный цикл
        sf::Clock clock;
        bool showDebugInfo = true;

        while (window.IsOpen()) {
            sf::Event event;
            while (window.PollEvent(event)) {
                if (event.type == sf::Event::KeyPressed) {
                    switch (event.key.code) {
                        case sf::Keyboard::Space:
                            player.TogglePause();
                            break;
                        case sf::Keyboard::Escape:
                            window.Close();
                            break;
                        case sf::Keyboard::Left:
                            player.Seek(-10);
                            break;
                        case sf::Keyboard::Right:
                            player.Seek(10);
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
                        case sf::Keyboard::D:
                            showDebugInfo = !showDebugInfo;
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

            // Обновляем и отображаем информацию о воспроизведении
            if (fontLoaded) {
                std::string statusInfo;
                if (player.IsPaused()) {
                    statusInfo = "[ПАУЗА]";
                } else if (player.IsFinished()) {
                    statusInfo = "[ЗАВЕРШЕНО]";
                } else {
                    statusInfo = "[ВОСПРОИЗВЕДЕНИЕ]";
                }

                std::string infoString = "Время: " + formatTime(player.GetCurrentTime()) +
                                         " / " + formatTime(player.GetDuration()) +
                                         "   Громкость: " + std::to_string(static_cast<int>(player.GetVolume())) + "%" +
                                         "   " + statusInfo;

                if (showDebugInfo) {
                    float fps = 1.0f / clock.restart().asSeconds();
                    infoString += "\nFPS: " + std::to_string(static_cast<int>(fps));
                }

                infoText.setString(infoString);

                // Создаем фон для текста
                sf::RectangleShape textBg;
                textBg.setSize(sf::Vector2f(window.GetSize().x, 40));
                textBg.setFillColor(sf::Color(0, 0, 0, 150));
                textBg.setPosition(0, 0);

                window.GetRenderWindow().draw(textBg);
                window.GetRenderWindow().draw(infoText);

                // Отображаем инструкции
                sf::RectangleShape instructionsBg;
                instructionsBg.setSize(sf::Vector2f(300, 120));
                instructionsBg.setFillColor(sf::Color(0, 0, 0, 150));
                instructionsBg.setPosition(5, window.GetSize().y - 155);

                window.GetRenderWindow().draw(instructionsBg);
                window.GetRenderWindow().draw(instructionsText);

                // Отображаем сообщение, если видео завершено
                if (player.IsFinished()) {
                    sf::Text endText;
                    endText.setFont(font);
                    endText.setCharacterSize(30);
                    endText.setFillColor(sf::Color::White);
                    endText.setString("Воспроизведение завершено");

                    sf::FloatRect textRect = endText.getLocalBounds();
                    endText.setPosition(
                        (window.GetSize().x - textRect.width) / 2.0f,
                        (window.GetSize().y - textRect.height) / 2.0f
                    );

                    sf::RectangleShape endBg;
                    endBg.setSize(sf::Vector2f(textRect.width + 40, textRect.height + 40));
                    endBg.setFillColor(sf::Color(0, 0, 0, 200));
                    endBg.setPosition(
                        endText.getPosition().x - 20,
                        endText.getPosition().y - 20
                    );

                    window.GetRenderWindow().draw(endBg);
                    window.GetRenderWindow().draw(endText);
                }
            }

            // Отображаем содержимое окна
            window.Display();
        }

        // Останавливаем воспроизведение
        player.Stop();

    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Использование: " << argv[0] << " <видеофайл> [--test-only]" << std::endl;
        return 1;
    }

    std::string filename = argv[1];
    bool testOnly = (argc > 2 && std::string(argv[2]) == "--test-only");

    if (testOnly) {
        // Запускаем только тестирование функциональности
        Player player;
        testPlayer(player, filename);
    } else {
        // Запускаем графическое тестирование
        runGraphicalTest(filename);
    }

    return 0;
}
