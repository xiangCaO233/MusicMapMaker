#ifndef MMM_COMPOSITE_HPP
#define MMM_COMPOSITE_HPP

#include "mmm/obj/Note.hpp"

class Composite : public Note {
   public:
    // 构造Composite
    using Note::Note;

    // 析构Composite
    ~Composite() override;

    // 打印用
    std::string toString() override;

    inline uint32_t total_duration() const { return total_duration_time; }

   private:
    uint32_t total_duration_time{0};
};

#endif  // MMM_COMPOSITE_HPP
