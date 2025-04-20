#include "Note.h"

Note::Note(uint32_t time) : HitObject(time) {
  object_type = HitObjectType::NOTE;
  note_type = NoteType::NOTE;
}

Note::~Note() {}

// 打印用
std::string Note::toString() { return ""; }
