#include "Hold.h"

Hold::Hold(uint32_t time, uint32_t holdtime) : Note(time), hold_time(holdtime) {
  type = NoteType::HOLD;
}

Hold::~Hold() = default;

// 打印用
std::string Hold::toString() { return ""; }
