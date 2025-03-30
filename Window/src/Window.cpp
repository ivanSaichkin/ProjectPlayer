#include "../include/Window.hpp"
#include <iostream>

Window::Window(int width, int height, const std::string& title)
    : title_(title),
      width_(width),
      height_(height),
      isFullscreen_(false),
      resizeCallback_(nullptr),
      closeCallback_(nullptr) {
}

Window::~Window() {
    if (window_.isOpen()) {
        window_.close();
    }
}

bool Window::Open() {
    try {
        CreateWindow();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error creating window: " << e.what() << std::endl;
        return false;
    }
}

void Window::CreateWindow() {
    // Создаем стили окна в зависимости от режима (полноэкранный или оконный)
    sf::Uint32 style = isFullscreen_ ? sf::Style::Fullscreen :
                                      (sf::Style::Resize | sf::Style::Close | sf::Style::Titlebar);

    if (window_.isOpen()) {
        window_.close();
    }

    // Создаем новые настройки для окна
    sf::ContextSettings settings;
    settings.antialiasingLevel = 4; // Включаем сглаживание

    window_.create(sf::VideoMode(width_, height_), title_, style, settings);

    // Устанавливаем вертикальную синхронизацию для более плавного отображения
    window_.setVerticalSyncEnabled(true);

    if (!isFullscreen_) {
        sf::VideoMode desktopMode = sf::VideoMode::getDesktopMode();
        window_.setPosition(sf::Vector2i(
            (desktopMode.width - width_) / 2,
            (desktopMode.height - height_) / 2
        ));
    }

    // Устанавливаем иконку окна (если нужно)
    // sf::Image icon;
    // if (icon.loadFromFile("icon.png")) {
    //     window_.setIcon(icon.getSize().x, icon.getSize().y, icon.getPixelsPtr());
    // }
}

void Window::Close() {
    if (window_.isOpen()) {
        window_.close();
    }
}

bool Window::IsOpen() const {
    return window_.isOpen();
}

bool Window::PollEvent(sf::Event& event) {
    bool eventPolled = window_.pollEvent(event);

    if (eventPolled) {
        // Обрабатываем событие изменения размера окна
        if (event.type == sf::Event::Resized) {
            width_ = event.size.width;
            height_ = event.size.height;

            // Обновляем вид окна, чтобы соответствовать новому размеру
            sf::FloatRect visibleArea(0, 0, width_, height_);
            window_.setView(sf::View(visibleArea));

            // Вызываем обработчик изменения размера, если он установлен
            if (resizeCallback_) {
                resizeCallback_(width_, height_);
            }
        }
        // Обрабатываем событие закрытия окна
        else if (event.type == sf::Event::Closed) {
            if (closeCallback_) {
                closeCallback_();
            }
            window_.close();
        }
    }

    return eventPolled;
}

void Window::Clear(const sf::Color& color) {
    window_.clear(color);
}

void Window::Display() {
    window_.display();
}

sf::RenderWindow& Window::GetRenderWindow() {
    return window_;
}

void Window::ScaleSprite(sf::Sprite& sprite, bool preserveRatio) {
    // Получаем текущий размер спрайта (размер текстуры)
    sf::Vector2u textureSize = sprite.getTexture()->getSize();
    float spriteWidth = static_cast<float>(textureSize.x);
    float spriteHeight = static_cast<float>(textureSize.y);

    // Получаем размер окна
    float windowWidth = static_cast<float>(width_);
    float windowHeight = static_cast<float>(height_);

    // Вычисляем коэффициенты масштабирования
    float scaleX = windowWidth / spriteWidth;
    float scaleY = windowHeight / spriteHeight;

    // Если нужно сохранить пропорции
    if (preserveRatio) {
        // Используем наименьший коэффициент для сохранения пропорций
        float scale = std::min(scaleX, scaleY);
        sprite.setScale(scale, scale);

        // Центрируем спрайт в окне
        float newWidth = spriteWidth * scale;
        float newHeight = spriteHeight * scale;
        sprite.setPosition((windowWidth - newWidth) / 2.0f, (windowHeight - newHeight) / 2.0f);
    } else {
        // Масштабируем по размерам окна без сохранения пропорций
        sprite.setScale(scaleX, scaleY);
        sprite.setPosition(0, 0);
    }
}

void Window::Resize(int width, int height) {
    width_ = width;
    height_ = height;

    if (!isFullscreen_) {
        window_.setSize(sf::Vector2u(width_, height_));

        // Центрируем окно после изменения размера
        sf::VideoMode desktopMode = sf::VideoMode::getDesktopMode();
        window_.setPosition(sf::Vector2i(
            (desktopMode.width - width_) / 2,
            (desktopMode.height - height_) / 2
        ));
    }

    // Обновляем вид окна
    sf::FloatRect visibleArea(0, 0, width_, height_);
    window_.setView(sf::View(visibleArea));

    // Вызываем обработчик изменения размера, если он установлен
    if (resizeCallback_) {
        resizeCallback_(width_, height_);
    }
}

sf::Vector2u Window::GetSize() const {
    return sf::Vector2u(width_, height_);
}

void Window::ToggleFullscreen() {
    isFullscreen_ = !isFullscreen_;

    // Если переключаемся в полноэкранный режим
    if (isFullscreen_) {
        // Сохраняем текущие размеры окна для возврата из полноэкранного режима
        sf::Vector2u currentSize = window_.getSize();
        width_ = currentSize.x;
        height_ = currentSize.y;

        // Получаем размеры рабочего стола
        sf::VideoMode desktopMode = sf::VideoMode::getDesktopMode();

        // Пересоздаем окно в полноэкранном режиме
        CreateWindow();
    } else {
        // Пересоздаем окно в оконном режиме с сохраненными размерами
        CreateWindow();
    }
}

void Window::SetResizeCallback(std::function<void(int, int)> callback) {
    resizeCallback_ = callback;
}

void Window::SetCloseCallback(std::function<void()> callback) {
    closeCallback_ = callback;
}
