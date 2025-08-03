#ifndef MMM_HOLD_HPP
#define MMM_HOLD_HPP

#include "mmm/obj/Note.hpp"

class Hold : public Note {
   public:
    // 构造Hold
    Hold();
    // 析构Hold
    ~Hold() override;

    inline uint32_t duration() const { return duration_time; }

    inline void set_duration(uint32_t duration) { duration_time = duration; }

   private:
    uint32_t duration_time;
};

#endif  // MMM_HOLD_HPP
