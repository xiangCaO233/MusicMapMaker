#include "Note.h"

Note::Note(uint32_t time) : HitObject(time) { type = NoteType::NOTE; }

Note::~Note() {}

// 打印用
std::string Note::toString() { return ""; }
