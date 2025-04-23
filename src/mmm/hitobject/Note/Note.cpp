#include "Note.h"

Note::Note(uint32_t time, int32_t orbit_pos)
    : HitObject(time), orbit(orbit_pos) {
  object_type = HitObjectType::NOTE;
  note_type = NoteType::NOTE;
}

Note::~Note() {}

// 打印用
std::string Note::toString() { return ""; }
