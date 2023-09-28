#ifndef ANIMATION_H
#define ANIMATION_H

#include <optional>
#include <stdexcept>
#include <functional>
#include <map>
#include <set>
#include <memory>
#include <utility>
#include "../entities/buildable.h"
#include "../scene_graph.h"
#include "../utils/transforms.h"
#include "scheduler.h"

namespace atk {

    /**
     * Abstract representation of a "thing" happening. May repeat an arbitrary number of times
     * or only fire on a condition, or not at all. Note that animations may not necessarily be
     * applied in order or continuously in a given sequence. Implementers may retain state for
     * example to cache the results of expensive operations, but should not assume that the
     * animate method will be called in any particular order.
     */
    class Animation {
    public:

        virtual void animate(ScheduleWindow& schedule_window) = 0;


        /**
         * The specified schedule window is opening. Animations should set
         * start state variables here.
         */
        virtual void activate(ScheduleWindow& scheduleWindow) = 0;

        /**
         * The specified schedule window is closing. Animations should set
         * their end state here.
         */
        virtual void terminate(ScheduleWindow& schedule_window) = 0;
    };

    /**
     * An animation that operates on a finite schedule window with known start and end timestamps.
     * In this case, all animation tasks will boil down to determining some 0:1 value representing
     * the current timestamp's position in the schedule window
     */
    class InterpolatedAnimation : public Animation {
    private:
        std::function<float(float)> interpolation_function;
        std::function<void(float)> action;

    public:
        InterpolatedAnimation(
                std::function<float(float)>  interpolation_function,
                std::function<void(float)>  action) :
            interpolation_function(std::move(interpolation_function)),
            action(std::move(action)) {

        }

        void animate(ScheduleWindow& scheduleWindow) override {
            float percent = scheduleWindow.percent_complete();
            float interpolated_percent = interpolation_function(percent);
            action(interpolated_percent);
        }

        void activate(ScheduleWindow &scheduleWindow) override {
            // nothing to do
        }

        void terminate(ScheduleWindow& schedule_window) override {
            action(interpolation_function(1.0f));
        }

        static std::function<float(float)> linear_interpolation() {
            return [](float val){ return val; };
        }

        static std::function<float(float)> reverse(const std::function<float(float)> input) {
            return [input](float val){
                return 1.0 - val;
            };
        }

        static std::function<float(float)> ease_out_interpolation() {
            return [](float val){
                auto ease_in = val * val;
                auto ease_out = 1.0f - (1.0f - val) * (1.0f - val);

                return ease_out;
            };
        }

        static std::function<float(float)> ease_in_out_interpolation() {
            return [](float val){
                auto ease_in = val * val;
                auto ease_out = 1.0f - (1.0f - val) * (1.0f - val);
                return std::lerp(ease_in, ease_out, val);
            };
        }
    };

    class InterplatedActions {
    public:

        /**
         * translation applied in local coordinates
         */
        static std::function<void(float)> x_translation(const float& x0, const float& x1, const std::weak_ptr<SceneNode>& node) {
            return [x0, x1, node](float v) {
                auto node_ptr = node.lock();

                auto current_y = TransformUtils::get_translation_part(node_ptr->transform()).second;

                TransformUtils::set_translation_part(
                        node_ptr->transform(),
                        std::lerp(x0, x1, v),
                        current_y);
            };
        }

        /**
         * Translation applied in local coordinates
         */
        static std::function<void(float)> y_translation(const float& y0, const float& y1, const std::weak_ptr<SceneNode>& node) {
            return [y0, y1, node](float v) {
                auto node_ptr = node.lock();

                auto current_x = TransformUtils::get_translation_part(node_ptr->transform()).first;

                TransformUtils::set_translation_part(
                        node_ptr->transform(),
                        current_x,
                        std::lerp(y0, y1, v));
            };
        }

        static std::function<void(float)> set_build_percent(const std::weak_ptr<SceneNode>& node) {
            return [node](float v) {
                node.lock()->modify<atk::Buildable>([v](atk::Buildable& b){
                    b.set_build_percent(v);
                });
            };
        }
    };
}

#endif