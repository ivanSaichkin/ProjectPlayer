#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include "../ProcessVideoPlayer/include/Player.hpp"
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

// Функция для отображения прогресса загрузки
void showLoadingProgress(float progress, sf::RenderWindow& window, sf::Font& font) {
    window.clear(sf::Color(20, 20, 20));

    // Текст "Загрузка..."
    sf::Text loadingText;
    loadingText.setFont(font);
    loadingText.setCharacterSize(24);
    loadingText.setFillColor(sf::Color::White);
    loadingText.setString("Загрузка видео...");

    // Центрируем текст
    sf::FloatRect textRect = loadingText.getLocalBounds();
    loadingText.setPosition(
        (window.getSize().x - textRect.width) / 2.0f,
        (window.getSize().y - textRect.height) / 2.0f - 50
    );

    // Создаем прогресс-бар
    sf::RectangleShape progressBar;
    progressBar.setSize(sf::Vector2f(window.getSize().x * 0.7f, 20));
    progressBar.setFillColor(sf::Color(50, 50, 50));
    progressBar.setPosition(
        window.getSize().x * 0.15f,
        (window.getSize().y - 20) / 2.0f + 20
    );

    // Индикатор прогресса
    sf::RectangleShape progressIndicator;
    progressIndicator.setSize(sf::Vector2f(progressBar.getSize().x * progress, 20));
    progressIndicator.setFillColor(sf::Color(50, 150, 255));
    progressIndicator.setPosition(progressBar.getPosition());

    // Текст с процентами
    sf::Text percentText;
    percentText.setFont(font);
    percentText.setCharacterSize(16);
    percentText.setFillColor(sf::Color::White);
    percentText.setString(std::to_string(static_cast<int>(progress * 100)) + "%");

    // Центрируем текст с процентами
    sf::FloatRect percentRect = percentText.getLocalBounds();
    percentText.setPosition(
        (window.getSize().x - percentRect.width) / 2.0f,
        progressBar.getPosition().y + progressBar.getSize().y + 10
    );

    // Отрисовываем все элементы
    window.draw(loadingText);
    window.draw(progressBar);
    window.draw(progressIndicator);
    window.draw(percentText);

    window.display();
}

int main(int argc, char* argv[]) {
    // Проверяем аргументы командной строки
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <video_file>" << std::endl;
        return 1;
    }

    std::string filename = argv[1];

    // Создаем временное окно для отображения прогресса загрузки
    sf::RenderWindow loadingWindow(sf::VideoMode(600, 200), "Загрузка видео", sf::Style::Titlebar);

    // Загружаем шрифт
    sf::Font font;
    bool fontLoaded = font.loadFromFile("Arial.ttf");

    if (!fontLoaded) {
        // Пробуем другие шрифты
        std::vector<std::string> fontPaths = {
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
    }

    // Создаем плеер
    Player player;

    try {
        // Загружаем и индексируем видео с отображением прогресса
        bool loaded = player.Load(filename, [&](float progress) {
            if (fontLoaded && loadingWindow.isOpen()) {
                showLoadingProgress(progress, loadingWindow, font);
            } else {
                std::cout << "Загрузка: " << static_cast<int>(progress * 100) << "%\r";
                std::cout.flush();
            }
        });

        // Закрываем окно загрузки
        if (loadingWindow.isOpen()) {
            loadingWindow.close();
        }

        if (!loaded) {
            std::cerr << "Не удалось загрузить видео" << std::endl;
            return 1;
        }

        std::cout << std::endl << "Видео успешно загружено!" << std::endl;

        // Получаем размеры видео
        int videoWidth = player.GetWidth();
        int videoHeight = player.GetHeight();

        // Создаем окно подходящего размера
        int windowWidth = videoWidth;
        int windowHeight = videoHeight;

        // Ограничиваем размер окна
        sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
        if (windowWidth > desktop.width * 0.8f || windowHeight > desktop.height * 0.8f) {
            float scale = std::min(
                (desktop.width * 0.8f) / windowWidth,
                (desktop.height * 0.8f) / windowHeight
            );
            windowWidth = static_cast<int>(windowWidth * scale);
            windowHeight = static_cast<int>(windowHeight * scale);
        }

        // Устанавливаем минимальный размер окна
        windowWidth = std::max(windowWidth, 640);
        windowHeight = std::max(windowHeight, 480);

        // Создаем окно
        Window window(windowWidth, windowHeight, "Видеоплеер - " + filename);
        if (!window.Open()) {
            std::cerr << "Не удалось создать окно" << std::endl;
            return 1;
        }

        // Создаем текст для отображения информации
        sf::Text infoText;
        if (fontLoaded) {
            infoText.setFont(font);
            infoText.setCharacterSize(20);
            infoText.setFillColor(sf::Color::White);
            infoText.setOutlineColor(sf::Color::Black);
            infoText.setOutlineThickness(1.0f);
            infoText.setPosition(10, 10);
        }

        // Создаем текст для инструкций
        sf::Text helpText;
        if (fontLoaded) {
            helpText.setFont(font);
            helpText.setCharacterSize(16);
            helpText.setFillColor(sf::Color::White);
            helpText.setPosition(10, window.GetSize().y - 120);

            helpText.setString(
                "Пробел: Воспроизведение/Пауза\n"
                "Стрелки влево/вправо: Перемотка -/+ 10с\n"
                "Стрелки вверх/вниз: Громкость +/-\n"
                "M: Включить/выключить звук\n"
                "F: Полноэкранный режим\n"
                "Esc: Выход"
            );
        }

        // Создаем ползунок прогресса
        sf::RectangleShape progressBar;
        progressBar.setSize(sf::Vector2f(window.GetSize().x - 20, 10));
        progressBar.setPosition(10, window.GetSize().y - 30);
        progressBar.setFillColor(sf::Color(100, 100, 100, 200));

        sf::RectangleShape progressIndicator;
        progressIndicator.setSize(sf::Vector2f(0, 10));
        progressIndicator.setPosition(10, window.GetSize().y - 30);
        progressIndicator.setFillColor(sf::Color(50, 150, 255, 200));

        // Устанавливаем начальную громкость
        player.SetVolume(80.0f);

        // Устанавливаем максимальное количество кадров в памяти
        // Для больших видео можно уменьшить, для маленьких увеличить
        player.SetMaxFramesInMemory(300);  // ~10 секунд при 30fps
        player.SetMaxAudioChunksInMemory(100);  // ~30 секунд аудио

        // Начинаем воспроизведение
        player.Play();

        // Основной цикл приложения
        sf::Clock frameClock;

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
                            player.Seek(player.GetCurrentTime() - 10.0);
                            break;

                        case sf::Keyboard::Right:
                            player.Seek(player.GetCurrentTime() + 10.0);
                            break;

                        case sf::Keyboard::Up:
                            player.SetVolume(player.GetVolume() + 5.0f);
                            break;

                        case sf::Keyboard::Down:
                            player.SetVolume(player.GetVolume() - 5.0f);
                            break;

                        case sf::Keyboard::M:
                            player.Mute(!player.IsMuted());
                            break;

                        case sf::Keyboard::F:
                            window.ToggleFullscreen();

                            // Обновляем положение элементов интерфейса
                            if (fontLoaded) {
                                helpText.setPosition(10, window.GetSize().y - 120);
                                progressBar.setSize(sf::Vector2f(window.GetSize().x - 20, 10));
                                progressBar.setPosition(10, window.GetSize().y - 30);
                            }
                            break;

                        case sf::Keyboard::R:
                            // Перезапуск воспроизведения
                            if (player.IsFinished()) {
                                player.Seek(0.0);
                                player.Play();
                            }
                            break;

                        default:
                            break;
                    }
                }
                else if (event.type == sf::Event::MouseButtonPressed) {
                    // Обработка клика на прогресс-бар для перемотки
                    if (event.mouseButton.button == sf::Mouse::Left) {
                        sf::Vector2i mousePos = sf::Mouse::getPosition(window.GetRenderWindow());
                        sf::FloatRect progressBarBounds = progressBar.getGlobalBounds();

                        if (progressBarBounds.contains(mousePos.x, mousePos.y)) {
                            float ratio = (mousePos.x - progressBarBounds.left) / progressBarBounds.width;
                            double seekTime = ratio * player.GetDuration();
                            player.Seek(seekTime);
                        }
                    }
                }
                else if (event.type == sf::Event::Resized) {
                    // Обновляем положение элементов интерфейса при изменении размера окна
                    if (fontLoaded) {
                        helpText.setPosition(10, window.GetSize().y - 120);
                        progressBar.setSize(sf::Vector2f(window.GetSize().x - 20, 10));
                        progressBar.setPosition(10, window.GetSize().y - 30);
                    }
                }
            }

            // Очищаем окно
            window.Clear(sf::Color(0, 0, 0));

            // Отрисовываем видео
            player.Draw(window);

            // Обновляем индикатор прогресса
            if (player.GetDuration() > 0) {
                float progress = static_cast<float>(player.GetCurrentTime() / player.GetDuration());
                progressIndicator.setSize(sf::Vector2f(progress * (window.GetSize().x - 20), 10));
            }

            // Отображаем информацию о воспроизведении
            if (fontLoaded) {
                std::string status;
                if (player.IsFinished()) {
                    status = "[Завершено]";
                } else if (player.IsPaused()) {
                    status = "[Пауза]";
                } else if (player.IsPlaying()) {
                    status = "[Воспроизведение]";
                } else {
                    status = "[Остановлено]";
                }

                std::string muteStatus = player.IsMuted() ? "[Без звука]" : "";

                std::stringstream ss;
                ss << formatTime(player.GetCurrentTime()) << " / "
                   << formatTime(player.GetDuration()) << "  "
                   << status << "  "
                   << "Громкость: " << static_cast<int>(player.GetVolume()) << "% "
                   << muteStatus;

                infoText.setString(ss.str());

                // Полупрозрачный фон для текста
                sf::RectangleShape textBackground;
                textBackground.setSize(sf::Vector2f(infoText.getLocalBounds().width + 20, 30));
                textBackground.setPosition(5, 5);
                textBackground.setFillColor(sf::Color(0, 0, 0, 150));

                // Полупрозрачный фон для инструкций
                sf::RectangleShape helpBackground;
                helpBackground.setSize(sf::Vector2f(helpText.getLocalBounds().width + 20, 110));
                helpBackground.setPosition(5, window.GetSize().y - 125);
                helpBackground.setFillColor(sf::Color(0, 0, 0, 150));

                // Отрисовываем интерфейс
                window.GetRenderWindow().draw(textBackground);
                window.GetRenderWindow().draw(infoText);
                window.GetRenderWindow().draw(helpBackground);
                window.GetRenderWindow().draw(helpText);
                window.GetRenderWindow().draw(progressBar);
                window.GetRenderWindow().draw(progressIndicator);

                // Если воспроизведение завершено, показываем сообщение
                if (player.IsFinished()) {
                    sf::Text endText;
                    endText.setFont(font);
                    endText.setCharacterSize(30);
                    endText.setFillColor(sf::Color::White);
                    endText.setOutlineColor(sf::Color::Black);
                    endText.setOutlineThickness(2.0f);
                    endText.setString("Воспроизведение завершено\nНажмите R для перезапуска или\nпробел для продолжения с начала");

                    // Центрируем текст
                    sf::FloatRect textRect = endText.getLocalBounds();
                    endText.setPosition(
                        (window.GetSize().x - textRect.width) / 2.0f,
                        (window.GetSize().y - textRect.height) / 2.0f
                    );

                    // Полупрозрачный фон
                    sf::RectangleShape endBackground;
                    endBackground.setSize(sf::Vector2f(textRect.width + 40, textRect.height + 30));
                    endBackground.setFillColor(sf::Color(0, 0, 0, 180));
                    endBackground.setPosition(
                        endText.getPosition().x - 20,
                        endText.getPosition().y - 15
                    );

                    window.GetRenderWindow().draw(endBackground);
                    window.GetRenderWindow().draw(endText);
                }
            }

            // Отображаем содержимое окна
            window.Display();

            // Ограничиваем FPS
            sf::Time frameTime = frameClock.restart();
            sf::Time minFrameTime = sf::milliseconds(16); // ~60 FPS

            if (frameTime < minFrameTime) {
                sf::sleep(minFrameTime - frameTime);
            }
        }

        // Останавливаем воспроизведение при выходе
        player.Stop();

    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
