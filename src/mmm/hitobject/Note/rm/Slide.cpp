#include "Slide.h"

#include "../../HitObject.h"
#include "../../canvas/map/generator/ObjectGenerator.h"

SlideEnd::SlideEnd(const std::shared_ptr<Slide>& head)
    : HitObject(head->timestamp), reference(head.get()) {
  object_type = HitObjectType::RMSLIDE;
  endorbit = head->orbit + head->slide_parameter;
}

SlideEnd::~SlideEnd() = default;

// 接收处理
// 滑键尾不处理
void SlideEnd::accept_generate(ObjectGenerator& generator) {}
void SlideEnd::accept_generate_preview(ObjectGenerator& generator) {}

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
std::string SlideEnd::toString() {
  return "SlideEnd{timestamp=" + std::to_string(timestamp) +
         ", endorbit=" + std::to_string(endorbit) +
         ", ref_timestamp=" + std::to_string(reference->timestamp) +
         ", ref_orbit=" + std::to_string(reference->orbit) + "}";
}

Slide::Slide(uint32_t time, int32_t orbit_pos, int32_t slide_par)
    : Note(time, orbit_pos), slide_parameter(slide_par) {
  object_type = HitObjectType::RMSLIDE;
  note_type = NoteType::SLIDE;
}

Slide::~Slide() = default;

// 接收处理
void Slide::accept_generate(ObjectGenerator& generator) {
  generator.generate(*this);
}
void Slide::accept_generate_preview(ObjectGenerator& generator) {
  generator.generate_preview(*this);
}

// 打印用
std::string Slide::toString() {
  return "Slide{timestamp=" + std::to_string(timestamp) +
         ", orbit=" + std::to_string(orbit) +
         ", slide_param=" + std::to_string(slide_parameter) + "}";
}

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
