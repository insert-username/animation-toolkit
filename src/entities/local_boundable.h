#ifndef ENTITIES_LOCAL_BOUNDABLE_H
#define ENTITIES_LOCAL_BOUNDABLE_H

#include <SFML/Graphics.hpp>

namespace atk {

    class LocalBoundable {
    public:
        virtual sf::FloatRect get_local_bounds() = 0;
    };

}

#endif