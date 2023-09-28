#ifndef ANIMATION_SCHEDULE_WINDOW_H
#define ANIMATION_SCHEDULE_WINDOW_H

#include <optional>
#include <stdexcept>
#include <functional>
#include <map>
#include <set>
#include <memory>
#include <utility>
#include "../entities/buildable.h"
#include "timestamp.h"

namespace atk {

    /**
     * Represents a single contiguous sequence where a schedule is ACTIVE. Contains information
     * (if known) about the schedule
     */
    struct ScheduleWindow {
        std::optional<Timestamp> first_active_timestamp;
        std::optional<Timestamp> last_active_timestamp;

        /**
         * Regardless of end or start times, the current timestamp is always known.
         */
        Timestamp current_timestamp;

        ScheduleWindow(
                std::optional<Timestamp> first_active_timestamp,
                std::optional<Timestamp> last_active_timestamp,
                Timestamp current_timestamp) :
                first_active_timestamp(first_active_timestamp),
                last_active_timestamp(last_active_timestamp),
                current_timestamp(current_timestamp) {

        }

        /**
         * @return this schedule window, with all params offset by the number of seconds.
         */
        ScheduleWindow get_offset_seconds(const float& seconds) {
            return {
                    first_active_timestamp.has_value() ? std::make_optional<Timestamp>(first_active_timestamp->get_offset_by_seconds(seconds)) : std::nullopt,
                    last_active_timestamp.has_value() ? std::make_optional<Timestamp>(last_active_timestamp->get_offset_by_seconds(seconds)) : std::nullopt,
                    current_timestamp.get_offset_by_seconds(seconds)};
        }

        [[nodiscard]] bool is_finite() const {
            return first_active_timestamp.has_value() && last_active_timestamp.has_value();
        }

        [[nodiscard]] bool is_instantaneous() const {
            return is_finite() && first_active_timestamp.value() == last_active_timestamp.value();
        }

        [[nodiscard]] float percent_complete() const {
            if (!first_active_timestamp.has_value() || !last_active_timestamp.has_value()) {
                throw std::runtime_error("This schedule window is not bounded");
            }

            return (current_timestamp.seconds - first_active_timestamp->seconds) /
                   (last_active_timestamp->seconds - first_active_timestamp->seconds);
        }
    };
}

#endif