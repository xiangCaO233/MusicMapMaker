#include "Hold.h"

Hold::Hold(uint32_t time, uint32_t holdtime) : Note(time), hold_time(holdtime) {
  type = HitObjectType::HOLD;
}

Hold::~Hold() = default;
