#include "HoldEnd.h"

HoldEnd::HoldEnd(const std::shared_ptr<Hold>& hold)
    : HitObject(hold->timestamp + hold->hold_time), reference(hold) {
  // 设置面条物件的面尾引用
  hold->hold_end_reference = this;
}

HoldEnd::~HoldEnd() = default;

// 打印用
std::string HoldEnd::toString() { return ""; }
