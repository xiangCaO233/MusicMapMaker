#include "HoldEnd.h"

HoldEnd::HoldEnd(const std::shared_ptr<Hold>& hold)
    : HitObject(hold->timestamp + hold->hold_time), reference(hold) {}

HoldEnd::~HoldEnd() = default;

// 打印用
std::string HoldEnd::toString() { return ""; }
