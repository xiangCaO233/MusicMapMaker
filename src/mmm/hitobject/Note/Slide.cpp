#include "Slide.h"

Slide::Slide(uint32_t time, int32_t slide_par)
    : Note(time), slide_parameter(slide_par) {
  type = HitObjectType::SLIDE;
}

Slide::~Slide() = default;
