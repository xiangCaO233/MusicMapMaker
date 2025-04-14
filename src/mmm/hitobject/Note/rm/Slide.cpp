#include "Slide.h"

Slide::Slide(uint32_t time, int32_t slide_par)
    : Note(time), slide_parameter(slide_par) {
  type = NoteType::SLIDE;
}

Slide::~Slide() = default;

// 打印用
std::string Slide::toString() { return ""; }
