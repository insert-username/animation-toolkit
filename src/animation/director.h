#ifndef ANIMATION_DIRECTOR_H
#define ANIMATION_DIRECTOR_H

#include "../scene_graph.h"
#include "../rendering/renderer.h"
#include "timeline.h"
#include "animation.h"
#include "sfml_clock_timer.h"
#include "../utils/sequencer.h"

namespace atk {


    class ArrangeDirector {
    private:
        std::weak_ptr<SceneNode> target;

        std::vector<std::shared_ptr<SceneNode>> nodes;
        std::vector<std::function<void(float)>> actions;
        float spacing = 10.0f;
        Sequencer x_sequencer;
        Sequencer y_sequencer;
    public:
        ArrangeDirector(
                const std::shared_ptr<SceneNode>& target,
                const std::vector<std::shared_ptr<SceneNode>>& nodes,
                Sequencer x_sequencer,
                Sequencer y_sequencer) :
                target(target),
                nodes(),
                x_sequencer(x_sequencer),
                y_sequencer(y_sequencer) {

            for (const auto& n : nodes) {
                this->nodes.push_back(n);
            }

        }

        void add_schedules(Timeline &timeline) {
            actions.clear();

            auto target_ptr = target.lock();

            auto target_position = target_ptr->local_to_world_transform().transformPoint(0, 0);

            // determine the width of the overall result
            float result_width = 0.0f;
            for (auto &s : nodes) {
                result_width += s->world_bounds_recursive().width;
            }
            result_width += spacing * (float)(nodes.size() - 1);

            float world_target_x = target_position.x - result_width * 0.5f;
            float world_target_y = target_position.y;
            int index = 0;
            for (auto &s : nodes) {
                // target local translation to apply to object, relative to the current transform
                // local transform needs to change from current val -> current val + relative transform
                auto local_target = s->world_to_local_transform().transformPoint(world_target_x, world_target_y);

                // current local translation applied to the object
                auto local_start = TransformUtils::get_translation_part(s->transform());

                //std::cout << "Local start: " << local_start.first << ", " << local_start.second << std::endl;
                //std::cout << "Local target: " << local_target.x << ", " << local_target.y << std::endl;

                auto element_bounds = s->world_bounds_recursive();

                timeline.add(
                        std::make_unique<FireOnceScheduler>(x_sequencer.start(index), x_sequencer.end(index)),
                        std::make_unique<InterpolatedAnimation>(
                            atk::InterpolatedAnimation::ease_in_out_interpolation(),
                            InterplatedActions::x_translation(
                                    local_start.first, local_start.first + local_target.x + element_bounds.width * 0.5f, s)));
                timeline.add(
                        std::make_unique<FireOnceScheduler>(y_sequencer.start(index), y_sequencer.end(index)),
                        std::make_unique<InterpolatedAnimation>(
                                atk::InterpolatedAnimation::ease_in_out_interpolation(),
                                InterplatedActions::y_translation(
                                     local_start.second, local_start.second + local_target.y, s)));

                world_target_x += s->world_bounds_recursive().width;
                world_target_x += spacing;

                index++;
            }
        }

    };

    class Director {
    private:
        using SceneNodePtr = std::shared_ptr<SceneNode>;

        std::shared_ptr<Timeline> timeline;
        SceneNodePtr root_node;
        std::shared_ptr<Renderer> renderer;

    public:
        Director(SceneNodePtr root_node,
                 std::shared_ptr<Timeline> timeline,
                 std::shared_ptr<Renderer> renderer) :
                    root_node(std::move(root_node)),
                    timeline(std::move(timeline)),
                    renderer(std::move(renderer)) {

        }

        void build(const SceneNodePtr& node,
                   const Sequencer sequencer = {0.0f, 0.5f, 0.4f}) {

            int index = 0;
            node->visit_recursive([&index, &sequencer, this](const SceneNodePtr& n){
                auto buildable = n->try_get_drawable_as<atk::Buildable>();

                if (buildable.has_value()) {
                    if (buildable != nullptr) {
                        buildable.value()->set_build_percent(0.0f);
                        timeline->add(
                                std::make_unique<FireOnceScheduler>(sequencer.start(index), sequencer.end(index)),
                                std::make_unique<atk::InterpolatedAnimation>(
                                        atk::InterpolatedAnimation::ease_in_out_interpolation(),
                                        atk::InterplatedActions::set_build_percent(n)));
                        index++;
                    }
                }
            });
        }

        void unbuild(const SceneNodePtr& node,
                   const Sequencer sequencer = {0.0f, 0.5f, 0.4f}) {

            int index = 0;
            node->visit_recursive([&index, &sequencer, this](const SceneNodePtr& n){
                auto buildable = n->try_get_drawable_as<atk::Buildable>();

                if (buildable.has_value()) {
                    if (buildable != nullptr) {
                        buildable.value()->set_build_percent(1.0f);
                        timeline->add(
                                std::make_unique<FireOnceScheduler>(sequencer.start(index), sequencer.end(index)),
                                std::make_unique<atk::InterpolatedAnimation>(
                                        atk::InterpolatedAnimation::reverse(atk::InterpolatedAnimation::ease_in_out_interpolation()),
                                        atk::InterplatedActions::set_build_percent(n)));
                        index++;
                    }
                }
            });
        }

        void arrange(
                const std::shared_ptr<SceneNode>& target,
                const std::vector<std::shared_ptr<SceneNode>>& nodes,
                const Sequencer& x_sequencer,
                const Sequencer& y_sequencer) {

            ArrangeDirector(target, nodes, x_sequencer, y_sequencer)
                .add_schedules(*timeline);
        }

        /**
         * does not wait for any events to finish.
         */
        void play_forever(Timer& timer) {
            timer.restart();
            while (true) {
                auto time = timer.get_time_seconds();
                if (timeline->update(time).all_schedulers_terminated) {
                    timeline->clear();
                    // don't return. play forever
                }

                if (!renderer->render(*root_node).was_successful) {
                    return;
                }
            }
        }

        void play(Timer& timer) {
            timer.restart();
            while (true) {
                auto time = timer.get_time_seconds();

                if (timeline->update(time).all_schedulers_terminated) {
                    timeline->clear();
                    return;
                }

                if (!renderer->render(*root_node).was_successful) {
                    return;
                }
            }
        }

    };

}

#endif