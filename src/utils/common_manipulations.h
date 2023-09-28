#ifndef UTILS_COMMON_MANIPULATIONS_H
#define UTILS_COMMON_MANIPULATIONS_H

#include "../scene_graph.h"
#include "../entities/buildable.h"

namespace atk {
    class CommonManipulations {
    public:
        static void set_unbuilt(const std::shared_ptr<SceneNode>& node) {
            node->visit_recursive([](std::shared_ptr<SceneNode> n){
                auto b = n->try_get_drawable_as<Buildable>();

                if (b.has_value()) {
                    b.value()->set_build_percent(0.0f);
                }
            });
        }

        static void set_built(const std::shared_ptr<SceneNode>& node) {
            node->visit_recursive([](std::shared_ptr<SceneNode> n){
                auto b = n->try_get_drawable_as<Buildable>();

                if (b.has_value()) {
                    b.value()->set_build_percent(1.0f);
                }
            });
        }

        static void set_z_order(const std::shared_ptr<SceneNode>& node, const int& value) {
            node->visit_recursive([value](std::shared_ptr<SceneNode> n){
                n->set_z_order(value);
            });
        }

    };
}

#endif