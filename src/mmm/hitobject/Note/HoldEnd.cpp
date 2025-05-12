#include "HoldEnd.h"

HoldEnd::HoldEnd(const std::shared_ptr<Hold>& hold)
    : HitObject(hold->timestamp + hold->hold_time), reference(hold.get()) {
    is_hold_end = true;
    object_type = HitObjectType::HOLDEND;

    // hold->hold_end_reference = this;
}
HoldEnd::HoldEnd() : HitObject(0), reference(nullptr) {
    is_hold_end = true;
    object_type = HitObjectType::HOLDEND;
}

HoldEnd::~HoldEnd() = default;

// 深拷贝
HoldEnd* HoldEnd::clone() {
    auto end = new HoldEnd;
    end->timestamp = timestamp;
    end->beatinfo = beatinfo;
    return end;
}

// 是否为相同物件
bool HoldEnd::equals(const std::shared_ptr<HitObject>& other) const {
    if (other->object_type != object_type) return false;
    auto end = std::static_pointer_cast<HoldEnd>(other);
    return std::abs(timestamp - end->timestamp) < 5;
}

// 接收处理
// 面尾不处理
void HoldEnd::accept_generate(ObjectGenerator& generator) {}
void HoldEnd::accept_generate_preview(ObjectGenerator& generator) {}

// 打印用
std::string HoldEnd::toString() {
    return "HoldEnd{timestamp=" + std::to_string(timestamp) +
           ", ref_timestamp=" + std::to_string(reference->timestamp) +
           ", ref_orbit=" + std::to_string(reference->orbit) + "}";
}

// 比较器使用
bool HoldEnd::lessThan(const HitObject* other) const {
    return timestamp < other->timestamp
               ? true
               : this->object_type < other->object_type;
}
