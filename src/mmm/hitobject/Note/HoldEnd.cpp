#include "HoldEnd.h"

HoldEnd::HoldEnd(const std::shared_ptr<Hold>& hold)
    : HitObject(hold->timestamp + hold->hold_time), reference(hold.get()) {
  object_type = HitObjectType::HOLDEND;

  // hold->hold_end_reference = this;
}

HoldEnd::~HoldEnd() = default;

// 接收处理
void HoldEnd::accept_generate(ObjectGenerator& generator) {
  // 面尾不处理
};

// 打印用
std::string HoldEnd::toString() {
  return "HoldEnd{timestamp=" + std::to_string(timestamp) +
         ", ref_timestamp=" + std::to_string(reference->timestamp) +
         ", ref_orbit=" + std::to_string(reference->orbit) + "}";
}

// 比较器使用
bool HoldEnd::lessThan(const HitObject* other) const {
  return timestamp < other->timestamp ? true
                                      : this->object_type < other->object_type;
}
