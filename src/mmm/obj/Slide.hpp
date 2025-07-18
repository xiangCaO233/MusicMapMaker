#ifndef MMM_SLIDE_HPP
#define MMM_SLIDE_HPP

#include "mmm/obj/Note.hpp"

class Slide : public Note {
   public:
    // 构造Slide
    Slide();
    // 析构Slide
    ~Slide() override;

    inline uint32_t delta_track() const { return dtrack; }

    inline void set_track_orbit(uint32_t delta_track) { dtrack = delta_track; }

   private:
    uint32_t dtrack;
};

#endif  // MMM_SLIDE_HPP
