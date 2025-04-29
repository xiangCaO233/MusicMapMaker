#include "Note.h"

#include <typeinfo>

#include "../../canvas/map/generator/ObjectGenerator.h"

Note::Note(uint32_t time, int32_t orbit_pos)
    : HitObject(time), orbit(orbit_pos) {
  is_note = true;

  object_type = HitObjectType::NOTE;
  note_type = NoteType::NOTE;
}

Note::~Note() {}

// 接收处理
void Note::accept_generate(ObjectGenerator& generator) {
  generator.generate(*this);
};

// 打印用
std::string Note::toString() { return ""; }

// 比较器使用
bool Note::lessThan(const HitObject* other) const {
  if (typeid(*this) != typeid(*other)) {
    // 类型不同
    return timestamp < other->timestamp
               ? true
               : this->object_type < other->object_type;
  } else {
    // 比较内容
    auto otherd = static_cast<const Note*>(other);
    if (timestamp < other->timestamp) {
      return true;
    } else {
      if (timestamp == other->timestamp) {
        // 二级比较轨道
        return orbit < otherd->orbit;
      } else {
        return false;
      }
    }
  }
}
