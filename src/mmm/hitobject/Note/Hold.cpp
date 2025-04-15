#include "Hold.h"

#include "src/mmm/hitobject/HitObject.h"

HoldEnd::HoldEnd(const std::shared_ptr<Hold>& hold)
    : HitObject(hold->timestamp + hold->hold_time), reference(hold) {}

HoldEnd::~HoldEnd() = default;

// 打印用
std::string HoldEnd::toString() { return ""; }

Hold::Hold(uint32_t time, uint32_t holdtime) : Note(time), hold_time(holdtime) {
  type = NoteType::HOLD;
}

Hold::~Hold() = default;

// 打印用
std::string Hold::toString() { return ""; }
