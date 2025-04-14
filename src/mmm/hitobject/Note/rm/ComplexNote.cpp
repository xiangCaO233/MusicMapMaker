#include "ComplexNote.h"

ComplexNote::ComplexNote(uint32_t time) : Note(time) {
  type = HitObjectType::COMPLEX;
}

ComplexNote::~ComplexNote() = default;

// 打印用
std::string ComplexNote::toString() { return ""; }
