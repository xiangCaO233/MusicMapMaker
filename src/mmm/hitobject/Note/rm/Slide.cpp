#include "Slide.h"

#include "../../HitObject.h"

SlideEnd::SlideEnd(const std::shared_ptr<Slide>& head)
    : HitObject(head->timestamp), reference(head.get()) {
  object_type = HitObjectType::RMSLIDE;
  endorbit = head->orbit + head->slide_parameter;
}

SlideEnd::~SlideEnd() = default;

// 比较器使用
bool SlideEnd::lessThan(const HitObject* other) const {
  if (typeid(*this) != typeid(other)) {
    // 类型不同
    return timestamp < other->timestamp
               ? true
               : this->object_type < other->object_type;
  } else {
    // 比较内容
    auto otherd = static_cast<const SlideEnd*>(other);
    if (timestamp < other->timestamp) {
      return true;
    } else {
      if (timestamp == other->timestamp) {
        // 二级比较结束轨道
        return endorbit < otherd->endorbit;
      } else {
        return false;
      }
    }
  }
}

// 打印用
std::string SlideEnd::toString() { return ""; }

Slide::Slide(uint32_t time, int32_t orbit_pos, int32_t slide_par)
    : Note(time, orbit_pos), slide_parameter(slide_par) {
  object_type = HitObjectType::RMSLIDE;
  note_type = NoteType::SLIDE;
}

Slide::~Slide() = default;

// 打印用
std::string Slide::toString() { return ""; }

// 比较器使用
bool Slide::lessThan(const HitObject* other) const {
  if (typeid(*this) != typeid(*other)) {
    // 类型不同
    return timestamp < other->timestamp
               ? true
               : this->object_type < other->object_type;
  } else {
    // 比较内容
    auto otherd = static_cast<const Slide*>(other);
    if (timestamp < other->timestamp) {
      return true;
    } else {
      if (timestamp == other->timestamp) {
        // 二级比较滑动参数
        if (slide_parameter < otherd->slide_parameter) {
          return true;
        } else {
          if (slide_parameter == otherd->slide_parameter) {
            // 三级比较结束轨道
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
