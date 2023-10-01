#ifndef ENTITIES_RING_H
#define ENTITIES_RING_H

#include <SFML/Graphics.hpp>

#include "buildable.h"
#include "local_boundable.h"
#include "../constants.h"

namespace atk {
    class Ring : public sf::Drawable, public Buildable, public LocalBoundable {



    };
}

#endif