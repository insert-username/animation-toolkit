#ifndef ANIMATION_SCHEDULER_H
#define ANIMATION_SCHEDULER_H

#include <optional>
#include <stdexcept>
#include <functional>
#include <map>
#include <set>
#include <memory>
#include <utility>
#include <math.h>
#include "../entities/buildable.h"
#include "timestamp.h"
#include "schedule_window.h"

namespace atk {

    class Scheduler {
    public:
        struct ScheduleState {

            enum State {
                /**
                 * Indicates that at any time after the specified one, this schedule may become active.
                 */
                PENDING,

                /**
                 * Indicates that at the specified time the schedule will be active.
                 */
                ACTIVE,

                /**
                 * Indicates that, at the specified time and all times after, the schedule will never
                 * become pending or active. Schedules are not required to ever become TERMINATED, for
                 * example if they recur or run on some condition, but they should if it is known that
                 * they will not be scheduled again at timestamp T.
                 *
                 * A terminated scheduler MUST be able to specify the timestamp representing the last
                 * active moment of the schedule for a given input timestamp T.
                 */
                TERMINATED
            };

            State state;

            std::optional<ScheduleWindow> window_if_present;

        private:
            explicit ScheduleState(const State& state) : state(state), window_if_present(std::nullopt) {
                if (state == ACTIVE || state == TERMINATED) {
                    throw std::runtime_error("window must be specified if state is active or terminated");
                }

            }

            ScheduleState(const State& state, const ScheduleWindow& window) :
                    state(state), window_if_present(window) {

                if (window_if_present.has_value() && (state != ACTIVE && state != TERMINATED)) {
                    throw std::runtime_error("Only active/terminated states may have a window");
                }

                if (state == TERMINATED && !window_if_present.value().last_active_timestamp.has_value()) {
                    throw std::runtime_error("A last active timestamp must be specified if the state is terminated");
                }
            }
        public:
            static ScheduleState active(const ScheduleWindow& window) {
                return {ACTIVE, window};
            }

            static ScheduleState terminated(const ScheduleWindow& window) {
                return {TERMINATED, window};
            }

            static ScheduleState pending() {
                return ScheduleState(PENDING);
            }
        };

        /**
         * @return the schedule state at a particular timestamp T. The timestamp is considered to be "global" in that
         * the scheduler may refer to external dependencies with the assumption that they are updated to reflect T.
         *
         * The scheduler should not make assumptions about changes between given invocations of this method. It is
         * possible that the dt between updates may vary, or may not be invoked in order (this is unlikely as it would
         * cause other things to break, but the scheduler should treat it as assumed)
         */
        virtual ScheduleState schedule_state(const Timestamp& timestamp) = 0;
    };

    class FireOnceScheduler : public Scheduler {
    private:

        float start_seconds;
        float end_seconds;

    public:
        FireOnceScheduler(const float& start_seconds, const float& end_seconds) :
                start_seconds(start_seconds),
                end_seconds(end_seconds) {

        }

        ScheduleState schedule_state(const Timestamp &timestamp) override {
            if (timestamp.seconds < start_seconds) {
                return ScheduleState::pending();
            } else if (timestamp.seconds > end_seconds) {
                return ScheduleState::terminated(
                        ScheduleWindow(std::make_optional<Timestamp>(start_seconds),
                                       std::make_optional<Timestamp>(end_seconds),
                                       timestamp));
            }

            return ScheduleState::active(ScheduleWindow(
                    std::optional<Timestamp>(start_seconds),
                    std::optional<Timestamp>(end_seconds),
                    timestamp));
        }

    };

    class RepeatScheduler : public Scheduler {
    private:
        std::unique_ptr<Scheduler> delegate;

        float interval;

    public:

        RepeatScheduler(const float& interval, std::unique_ptr<Scheduler> delegate) :
                delegate(std::move(delegate)), interval(interval) {

        }

        /**
         * "trick" the delegate scheduler into thinking it is being re-fired over and over.
         */
        ScheduleState schedule_state(const Timestamp &timestamp) override {
            Timestamp modified_timestamp(std::fmod(timestamp.seconds, interval));

            return delegate->schedule_state(modified_timestamp);
        }
    };

    class PingPongScheduler : public Scheduler {
    private:
        std::shared_ptr<Scheduler> delegate;

        float interval;

    public:

        PingPongScheduler(const float& interval, std::shared_ptr<Scheduler> delegate) :
                delegate(std::move(delegate)), interval(interval) {

        }

        ScheduleState schedule_state(const Timestamp &timestamp) override {
            float double_interval = std::fmod(timestamp.seconds, 2.0 * interval);

            float ping_pong_interval = double_interval;

            if (double_interval > interval) {
                double_interval -= interval;
                ping_pong_interval = interval - double_interval;
            }

            Timestamp modified_timestamp(ping_pong_interval);

            return delegate->schedule_state(modified_timestamp);
        }
    };

    class DelayScheduler : public Scheduler {
    private:
        std::unique_ptr<Scheduler> delegate;

        float delay;

    public:

        DelayScheduler(const float& delay, std::unique_ptr<Scheduler> delegate) :
                delegate(std::move(delegate)), delay(delay) {

        }

        /**
         * "trick" the delegate scheduler into thinking it is being re-fired over and over.
         */
        ScheduleState schedule_state(const Timestamp &timestamp) override {
            Timestamp modified_timestamp(timestamp.seconds - delay);

            return delegate->schedule_state(modified_timestamp);
        }
    };

    /**
     * Scheduler that becomes enabled when a delegated scheduler is TERMINATED.
     */
    class SequencedScheduler : public Scheduler {
    private:
        std::shared_ptr<Scheduler> first;

        std::shared_ptr<Scheduler> delegate;

        std::string name;

    public:
        SequencedScheduler(std::shared_ptr<Scheduler> first, std::shared_ptr<Scheduler> delegate, std::string name) :
                first(std::move(first)), delegate(std::move(delegate)), name(name) {

        }

        ScheduleState schedule_state(const Timestamp &timestamp) override {
            auto first_schedule = first->schedule_state(timestamp);


            // this sequence only becomes active once the previous one is terminated.
            if (first_schedule.state != Scheduler::ScheduleState::TERMINATED) {
                return ScheduleState::pending();
            }

            // determine the timing offset. this sequence changes the t coordinate to zero for the delegate
            // once the previous sequence is terminated
            auto offset_seconds = first_schedule.window_if_present.value().last_active_timestamp.value().seconds;

            auto offset_timestamp = timestamp.get_offset_by_seconds(-offset_seconds);

            auto result = delegate->schedule_state(offset_timestamp);

            // translate the results back into the current timetamp
            result.window_if_present = result.window_if_present.has_value() ?
                                       std::make_optional<ScheduleWindow>(
                                               result.window_if_present.value().get_offset_seconds(offset_seconds)) :
                                       std::nullopt;

            return result;
        }
    };
}

#endif