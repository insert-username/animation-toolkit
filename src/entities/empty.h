#ifndef ENTITIES_EMPTY_H
#define ENTITIES_EMPTY_H

#include "local_boundable.h"

namespace atk {

    /**
     * Empty drawable, use for scene nodes where nothing is to be rendered.
     */
    class Empty: public sf::Drawable, public atk::LocalBoundable {

    protected:
        void draw(sf::RenderTarget &target, sf::RenderStates states) const override {
        }

    public:
        sf::FloatRect get_local_bounds() override {
            return {0, 0, 0, 0};
        }
    };

}

#endif