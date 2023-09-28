#ifndef UTILS_COLOR_H
#define UTILS_COLOR_H

#include <SFML/Graphics.hpp>
#include <cmath>

namespace atk {

    class ColorUtils {
    public:
        static sf::Color lerp(float frac, const sf::Color &from, const sf::Color &to) {
            frac = std::clamp(frac, 0.0f, 1.0f);

            sf::Color result;

            result.a = std::lerp(from.a, to.a, frac);
            result.b = std::lerp(from.b, to.b, frac);
            result.r = std::lerp(from.r, to.r, frac);
            result.g = std::lerp(from.g, to.g, frac);

            return result;
        }
    };

}

#endif