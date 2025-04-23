#include "Hold.h"

Hold::Hold(uint32_t time, uint32_t holdtime, int32_t orbit_pos)
    : Note(time, orbit_pos), hold_time(holdtime) {
  object_type = HitObjectType::HOLD;
  note_type = NoteType::HOLD;
}

Hold::~Hold() = default;

// 打印用
std::string Hold::toString() { return ""; }
