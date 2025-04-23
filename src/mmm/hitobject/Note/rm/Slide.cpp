#include "Slide.h"

#include "../../HitObject.h"

SlideEnd::SlideEnd(const std::shared_ptr<Slide> &head)
    : HitObject(head->timestamp), reference(head) {
  object_type = HitObjectType::RMSLIDE;
  endorbit = head->orbit + head->slide_parameter;
}

SlideEnd::~SlideEnd() = default;

// 打印用
std::string SlideEnd::toString() { return ""; }

Slide::Slide(uint32_t time, int32_t orbit_pos, int32_t slide_par)
    : Note(time, orbit_pos), slide_parameter(slide_par) {
  object_type = HitObjectType::RMSLIDE;
  note_type = NoteType::SLIDE;
}

Slide::~Slide() = default;

// 打印用
std::string Slide::toString() { return ""; }
