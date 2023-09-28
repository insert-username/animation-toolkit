#ifndef UTILS_SEQUENCER_H
#define UTILS_SEQUENCER_H

#include <stdexcept>

namespace atk {

    /**
     * Utility class for computing overlapping sequences.
     */
    class Sequencer {
    public:
        float offset = 0;

        float period = 1;
        float overlap = 0;

        [[nodiscard]] float start(const int& element) const {
            float interval = period - overlap;
            return interval * (float)element + offset;
        }

        [[nodiscard]] float end(const int& element) const {
            float interval = period - overlap;
            return interval * (float)element + period + offset;
        }

        static Sequencer filling_interval(const int& count,
                                          const float& duration,
                                          const float& overlap) {
            if (count < 2) {
                throw std::runtime_error("A sequence must consist of at least 2 elements.");
            }

            // end time for (n - 1) th element should be == interval
            // period interval / count

            auto result = Sequencer();

            float interval = duration / (float)(count - 1);

            result.period = interval + overlap;
            result.overlap = overlap;

            return result;
        }
    };
}


#endif