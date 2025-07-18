#ifndef MMM_HOLD_HPP
#define MMM_HOLD_HPP

#include "mmm/obj/Note.hpp"

class Hold : public Note {
   public:
    // 构造Hold
    Hold();
    // 析构Hold
    ~Hold() override;

    inline uint32_t durationtime() const { return duration; }

    inline void set_durationtime(uint32_t time) {
        duration = (time >= 0 ? time : 0);
    }

   private:
    uint32_t duration;
};

#endif  // MMM_HOLD_HPP
