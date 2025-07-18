#ifndef MMM_COMPOSITE_HPP
#define MMM_COMPOSITE_HPP

#include "mmm/obj/Note.hpp"

class Composite : public Note {
   public:
    // 构造Composite
    Composite();

    // 析构Composite
    ~Composite() override;

    inline uint32_t total_durationtime() const { return total_duration; }

   private:
    uint32_t total_duration;
};

#endif  // MMM_COMPOSITE_HPP
