#ifndef MMM_COMPOSITE_HPP
#define MMM_COMPOSITE_HPP

#include "mmm/obj/Note.hpp"

class Composite : public Note {
   public:
    // 构造Composite
    Composite();

    // 析构Composite
    ~Composite() override;

    inline uint32_t total_duration() const { return total_duration_time; }

   private:
    uint32_t total_duration_time;
};

#endif  // MMM_COMPOSITE_HPP
