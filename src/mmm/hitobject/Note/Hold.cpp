#include "Hold.h"

Hold::Hold(uint32_t time, int32_t orbit_pos, uint32_t holdtime)
    : Note(time, orbit_pos), hold_time(holdtime) {
  object_type = HitObjectType::HOLD;
  note_type = NoteType::HOLD;
}

Hold::~Hold() = default;

// 打印用
std::string Hold::toString() { return ""; }

// 比较器使用
bool Hold::lessThan(const HitObject* other) const {
  if (typeid(*this) != typeid(*other)) {
    // 类型不同
    return timestamp < other->timestamp
               ? true
               : this->object_type < other->object_type;
  } else {
    // 比较内容
    auto otherd = static_cast<const Hold*>(other);
    if (timestamp < other->timestamp) {
      return true;
    } else {
      if (timestamp == other->timestamp) {
        if (hold_time < otherd->hold_time) {
          return true;
        } else {
          if (hold_time == otherd->hold_time) {
            // 三级比较轨道
            return orbit < otherd->orbit;
          } else {
            return false;
          }
        }
      } else {
        return false;
      }
    }
  }
}
