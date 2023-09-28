#ifndef ENTITIES_H
#define ENTITIES_H

#include <SFML/Graphics.hpp>

namespace atk {
    /**
     * Indicates that an animation supports a progress based (0-1) value "build" sequence. For e.g. a line this could
     * be the percent that is rendered along the length. This is useful for smoothly introducing new objects to a scene.
     */
    class Buildable {
    public:
        virtual float get_build_percent() = 0;

        virtual void set_build_percent(const float& build_percent) = 0;
    };

}

#endif