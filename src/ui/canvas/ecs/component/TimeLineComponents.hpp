#ifndef MMM_TIMELINECOMPONENTS_HPP
#define MMM_TIMELINECOMPONENTS_HPP

#include <cstdint>

struct BeatComponent {
    uint32_t divisors;
    double beatLength;
    uint32_t beat_index;
};

#endif  // MMM_TIMELINECOMPONENTS_HPP
