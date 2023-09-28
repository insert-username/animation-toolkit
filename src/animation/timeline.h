#ifndef ANIMATION_TIMELINE_H
#define ANIMATION_TIMELINE_H

#include <optional>
#include <stdexcept>
#include <functional>
#include <map>
#include <set>
#include <memory>
#include <utility>
#include "../entities/buildable.h"
#include "scheduler.h"

namespace atk {

    /**
     * Collects and coordinates animations using schedulers.
     */
    class Timeline {
    public:
        struct UpdateResult {
            bool all_schedulers_terminated;
        };

    private:
        using SchedulerPtr = std::shared_ptr<Scheduler>;
        using AnimPtr = std::shared_ptr<Animation>;

        std::map<SchedulerPtr, AnimPtr> timeline;
        std::vector<SchedulerPtr> timeline_key_order;

        std::map<AnimPtr, ScheduleWindow> active_animations;

    public:

        void clear() {
            timeline.clear();
            timeline_key_order.clear();
            active_animations.clear();
        }

        UpdateResult update(float new_time_seconds) {
            const Timestamp timestamp {new_time_seconds};

            /**
             * First iterate over terminated animations, to allow them to set their end
             * states.
             */
            std::set<SchedulerPtr> terminated_schedulers;
            for (const auto& scheduler_ptr : timeline_key_order) {


                auto anim_ptr = timeline[scheduler_ptr];

                auto schedule_state = scheduler_ptr->schedule_state(timestamp);
                if (schedule_state.state == Scheduler::ScheduleState::TERMINATED) {
                    terminated_schedulers.insert(scheduler_ptr);

                    if (active_animations.contains(anim_ptr)) {
                        anim_ptr->terminate(active_animations.at(anim_ptr));
                        active_animations.erase(anim_ptr);
                    }

                }
            }

            bool all_terminated = true;

            for (const auto& scheduler_ptr : timeline_key_order) {
                auto anim_ptr = timeline[scheduler_ptr];

                if (terminated_schedulers.contains(scheduler_ptr)) {
                    // terminated. nothing to do.
                    continue;
                }

                all_terminated = false;
                auto schedule_state = scheduler_ptr->schedule_state(timestamp);

                if (schedule_state.state == Scheduler::ScheduleState::ACTIVE) {

                    // activate the animation if this is it's first ACTIVE frame
                    if (!active_animations.contains(anim_ptr)) {
                        anim_ptr->activate(schedule_state.window_if_present.value());
                    }

                    anim_ptr->animate(schedule_state.window_if_present.value());
                    active_animations.emplace(anim_ptr, schedule_state.window_if_present.value());
                } else if (schedule_state.state != Scheduler::ScheduleState::PENDING) {
                    throw std::runtime_error("Internal error");
                }
            }

            return UpdateResult {all_terminated};
        }

        void add(const std::shared_ptr<Scheduler>& scheduler, std::shared_ptr<Animation> animation) {
            timeline[scheduler] = std::move(animation);
            timeline_key_order.push_back(scheduler);
        }
    };
}

#endif