#include "Hold.h"

Hold::Hold(uint32_t time, uint32_t holdtime) : Note(time), hold_time(holdtime) {
  object_type = HitObjectType::HOLD;
  note_type = NoteType::HOLD;
}

Hold::~Hold() = default;

// 打印用
std::string Hold::toString() { return ""; }
