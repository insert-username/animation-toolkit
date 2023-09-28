#ifndef UTIL_BOUNDS_H
#define UTIL_BOUNDS_H

#include <SFML/Graphics.hpp>

namespace atk {
    class BoundsUtil {
    public:
        static sf::FloatRect combine(const sf::FloatRect& r0, const sf::FloatRect& r1) {
            float top = std::min(r0.top, r1.top);
            float left = std::min(r0.left, r1.left);

            float bottom = std::max(r0.top + r0.height, r1.top + r1.height);
            float right = std::max(r0.left + r0.width, r1.left + r1.width);

            return sf::FloatRect(top, left, bottom - top, right - left);
        }
    };
}


#endif