#ifndef MMM_BEAT_HPP
#define MMM_BEAT_HPP

#include <cstdint>

class Beat {
   public:
    Beat();
    Beat(Beat &&) = default;
    Beat(const Beat &) = default;
    Beat &operator=(Beat &&) = default;
    Beat &operator=(const Beat &) = default;
    virtual ~Beat();

    // 拍开始时间
    uint32_t beat_start;
};

#endif  // !MMM_BEAT_HPP
