#ifndef UTIL_TRANSFORMS_H
#define UTIL_TRANSFORMS_H

#include <SFML/Graphics.hpp>

namespace atk {

    class TransformUtils {
    public:
        static std::pair<float, float> get_translation_part(const sf::Transform &t) {
            float current_x = t.getMatrix()[12];
            float current_y = t.getMatrix()[13];

            return std::make_pair(current_x, current_y);
        }

        static void set_translation_part(sf::Transform& t, const float& dx, const float& dy) {
            auto position = get_translation_part(t);
            t.translate(dx - position.first, dy - position.second);
        }

    };

}


#endif