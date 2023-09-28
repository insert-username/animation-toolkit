#ifndef ENTITIES_GRID_H
#define ENTITIES_GRID_H

#include "buildable.h"
#include "curve.h"
#include "../constants.h"
#include "../utils/bounds.h"

namespace atk {

    class Grid {
    public:
        static std::shared_ptr<SceneNode> build(
                std::shared_ptr<ShaderCache> shader_cache,
                int x_count, int y_count, float x_increment, float y_increment) {
            auto result = std::make_shared<SceneNode>();

            auto v_lines = result->add("v_lines");
            auto h_lines = result->add("h_lines");


            const float width = x_increment * (float)(x_count);
            const float height = y_increment * (float)(y_count);

            for (int i = 0; i <= y_count; i++) {
                std::stringstream ss;
                ss << i;
                float y = y_increment * (float)i;
                h_lines->add(ss.str(),
                        std::make_unique<Curve>(
                        0, width,
                        shader_cache,
                        [y](float u){ return std::make_pair(u, y); },
                        10));
            }

            for (int i = 0; i <= x_count; i++) {
                std::stringstream ss;
                ss << i;
                float x = x_increment * (float)i;
                v_lines->add(ss.str(),
                        std::make_unique<Curve>(
                        0, height,
                        shader_cache,
                        [x](float u){ return std::make_pair(x, u); },
                        10));
            }

            return result;
        }

    };
}

#endif