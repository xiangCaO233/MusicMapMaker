#include "ComplexNote.h"

ComplexNote::ComplexNote(uint32_t time, int32_t orbit_pos)
    : Note(time, orbit_pos) {
  object_type = HitObjectType::RMCOMPLEX;
  note_type = NoteType::COMPLEX;
}

ComplexNote::~ComplexNote() = default;

// 打印用
std::string ComplexNote::toString() {
  return "ComplexNote{timestamp=" + std::to_string(timestamp) +
         ", orbit=" + std::to_string(orbit) +
         ", child_count=" + std::to_string(child_notes.size()) + "}";
}
