#ifndef UTILS_PROPORTIONAL_QUANTITY_H
#define UTILS_PROPORTIONAL_QUANTITY_H

#include <optional>
#include <math.h>

namespace atk {
    struct ProportionalQuantity {
        std::optional<float> min_absolute = std::nullopt;
        std::optional<float> max_absolute = std::nullopt;

        float proportion = 1.0f;
        float offset = 0.0f;

        ProportionalQuantity(std::optional<float> min_absolute, std::optional<float> max_absolute, float proportion, float offset) :
            min_absolute(min_absolute), max_absolute(max_absolute), proportion(proportion), offset(offset) {

        }

        float get_adjusted(const float& value) const {
            float result = proportion * value + offset;
            result = max_absolute.has_value() ? std::min(max_absolute.value(), result) : result;
            result = min_absolute.has_value() ? std::max(min_absolute.value(), result) : result;

            return result;
        }
    };
}

#endif