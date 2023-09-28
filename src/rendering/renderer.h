#ifndef RENDERING_RENDERER_H
#define RENDERING_RENDERER_H

#include "../scene_graph.h"

namespace atk {
    class Renderer {
    public:
        struct Result {
            bool was_successful;
        };

        virtual Result render(SceneNode& scene) = 0;
    };

    class WindowRenderer: public Renderer {
    private:
        std::unique_ptr<sf::RenderWindow> window;

        sf::Color _background_color;

        bool debug;

    public:
        WindowRenderer(int width, int height, const sf::Color& background_color, bool debug=false) :
        _background_color(background_color), debug(debug) {
            sf::ContextSettings settings;
            settings.antialiasingLevel = 8;
            window = std::make_unique<sf::RenderWindow>(
                    sf::VideoMode(width, height),
                    "Hello World",
                    sf::Style::Default,
                    settings);
            //window->setFramerateLimit(60);
        }

        Result render(SceneNode &scene) override {
            if (!window->isOpen()) {
                return Result {false};
            }

            sf::Event event;
            while (window->pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    window->close();
                }
            }

            window->clear(_background_color);

            scene.render([this](const sf::Drawable& d, const sf::Transform& t){
                window->draw(d, t);
            });

            if (debug) {
                sf::RectangleShape outline;
                outline.setFillColor(sf::Color::Transparent);
                outline.setOutlineThickness(1);
                outline.setOutlineColor(sf::Color::Red);

                scene.visit_recursive([&outline, this](auto s) {
                    auto bounds = s->world_bounds_recursive();
                    outline.setPosition(bounds.left, bounds.top);
                    outline.setSize(sf::Vector2f(bounds.width, bounds.height));
                    window->draw(outline);
                });
            }

            window->display();

            return Result{true};
        }
    };
}

#endif