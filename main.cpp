#include <SFML/Graphics.hpp>

using namespace sf;

int main() {
    RenderWindow window(VideoMode(600, 400), L"Новый проект", Style::Default);

    window.setVerticalSyncEnabled(true);

    sf::VertexArray triangle(Triangles, 3);
    triangle[0].position = Vector2f(300, 50);
    triangle[0].color = Color::Red;

    triangle[1].position = Vector2f(100, 350); // Левая нижняя вершина
    triangle[1].color = Color::Green;

    triangle[2].position = Vector2f(500, 350); // Правая нижняя вершина
    triangle[2].color = Color::Blue;

    RectangleShape rectangle(Vector2f(600, 400));
    rectangle.setPosition(0, 0);
    rectangle.setFillColor(Color::Black);

    while (window.isOpen())
    {
        Event event;
        while (window.pollEvent(event))
        {
            if (event.type == Event::Closed)
                window.close();
        }

        window.clear(Color::Yellow);
        window.draw(rectangle);
        window.draw(triangle);
        window.display();
    }
    return 0;
}
