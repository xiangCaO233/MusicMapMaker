#include "Slide.h"

#include "src/mmm/hitobject/HitObject.h"

SlideEnd::SlideEnd(const std::shared_ptr<Slide> &head)
    : HitObject(head->timestamp), reference(head) {
  endorbit = head->orbit + head->slide_parameter;
}

SlideEnd::~SlideEnd() = default;

// 打印用
std::string SlideEnd::toString() { return ""; }

Slide::Slide(uint32_t time, int32_t slide_par)
    : Note(time), slide_parameter(slide_par) {
  type = NoteType::SLIDE;
}

Slide::~Slide() = default;

// 打印用
std::string Slide::toString() { return ""; }
