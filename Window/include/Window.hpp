#ifndef WINDOW_HPP
#define WINDOW_HPP

#include "SFML/Graphics.hpp"
#include <string>
#include <functional>

class Window {
public:
    Window(int width, int height, const std::string& title);
    ~Window();

    bool Open();
    void Close();
    bool IsOpen() const;

    bool PollEvent(sf::Event& event);
    void Clear(const sf::Color& color = sf::Color::Black);
    void Display();

    sf::RenderWindow& GetRenderWindow();
    sf::Vector2u GetSize() const;

    void ScaleSprite(sf::Sprite& sprite, bool preserveRatio = true);
    void Resize(int width, int height);
    void ToggleFullscreen();

    void SetResizeCallback(std::function<void(int, int)> callback);
    void SetCloseCallback(std::function<void()> callback);

private:
    void CreateWindow();

    sf::RenderWindow window_;          // Объект окна SFML
    std::string title_;                // Заголовок окна
    int width_;                        // Ширина окна
    int height_;                       // Высота окна
    bool isFullscreen_;                // Флаг полноэкранного режима

    // Обработчики событий
    std::function<void(int, int)> resizeCallback_;
    std::function<void()> closeCallback_;
};


#endif
