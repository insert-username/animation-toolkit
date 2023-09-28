#ifndef ANIMATION_SFML_CLOCK_TIMER_H
#define ANIMATION_SFML_CLOCK_TIMER_H

#include <functional>
#include <SFML/Graphics.hpp>

namespace atk {
    class Timer {
    public:
        virtual float get_time_seconds() = 0;
        virtual void restart() = 0;
    };

    class SFMLClockTimer : public Timer {
    private:
        sf::Clock clock;
        float scale = 1.0f;
    public:
        void set_scale(const float& new_scale) {
            scale = new_scale;
        }

        float get_time_seconds() override {
            return clock.getElapsedTime().asSeconds() * scale;
        }

        void restart() override {
            clock.restart();
        }
    };
}

#endif