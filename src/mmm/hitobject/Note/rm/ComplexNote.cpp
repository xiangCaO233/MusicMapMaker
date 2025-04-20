#include "ComplexNote.h"

ComplexNote::ComplexNote(uint32_t time) : Note(time) {
  object_type = HitObjectType::RMCOMPLEX;
  note_type = NoteType::COMPLEX;
}

ComplexNote::~ComplexNote() = default;

// 打印用
std::string ComplexNote::toString() { return ""; }
