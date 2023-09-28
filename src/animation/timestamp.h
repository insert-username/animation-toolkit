#ifndef ANIMATION_TIMESTAMP_H
#define ANIMATION_TIMESTAMP_H

#include <optional>
#include <stdexcept>
#include <functional>
#include <map>
#include <set>
#include <memory>
#include <utility>
#include "../entities/buildable.h"

namespace atk {

    struct Timestamp {
        float seconds;

        explicit Timestamp(const float& seconds) : seconds(seconds) {

        }

        Timestamp get_offset_by_seconds(const float& seconds_offset) const {
            return Timestamp(this->seconds + seconds_offset);
        }

        friend bool operator ==(const Timestamp& t0, const Timestamp& t1) {
            return t0.seconds == t1.seconds;
        }
    };

}

#endif