#ifndef MMM_HOLD_HPP
#define MMM_HOLD_HPP

#include "mmm/obj/Note.hpp"

class Hold : public Note {
   public:
    // 构造Hold
    using Note::Note;
    // 析构Hold
    ~Hold() override = default;

    // 打印用
    std::string toString() const override { return ""; };

    inline uint32_t duration() const { return duration_time; }

    inline void set_duration(uint32_t duration) { duration_time = duration; }

   private:
    uint32_t duration_time{0};
};

#endif  // MMM_HOLD_HPP
