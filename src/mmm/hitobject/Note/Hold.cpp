#include "Hold.h"

#include "../../canvas/map/generator/ObjectGenerator.h"

Hold::Hold(uint32_t time, int32_t orbit_pos, uint32_t holdtime)
    : Note(time, orbit_pos), hold_time(holdtime) {
  object_type = HitObjectType::HOLD;
  note_type = NoteType::HOLD;
}

Hold::~Hold() = default;

// 深拷贝
Hold* Hold::clone() {
  auto hold = new Hold(timestamp, orbit, hold_time);
  hold->compinfo = compinfo;
  return hold;
}

// 是否为相同物件
bool Hold::equals(const std::shared_ptr<HitObject>& other) const {
  auto onote = std::dynamic_pointer_cast<Note>(other);
  if (!onote) return false;
  if (note_type != onote->note_type) return false;
  auto ohold = std::static_pointer_cast<Hold>(onote);
  return std::abs(timestamp - ohold->timestamp) < 5 && orbit == ohold->orbit &&
         std::fabs(hold_time - ohold->hold_time) < 5;
}

// 接收处理
void Hold::accept_generate(ObjectGenerator& generator) {
  generator.generate(*this);
}
void Hold::accept_generate_preview(ObjectGenerator& generator) {
  generator.generate_preview(*this);
}

// 打印用
std::string Hold::toString() {
  return "Hold{timestamp=" + std::to_string(timestamp) +
         ", orbit=" + std::to_string(orbit) +
         ", hold_time=" + std::to_string(hold_time) + "}";
}

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
