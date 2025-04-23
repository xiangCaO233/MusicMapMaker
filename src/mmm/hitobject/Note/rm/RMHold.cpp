#include "RMHold.h"

// 构造RMHold
RMHold::RMHold(uint32_t time, int32_t orbit_pos, uint32_t holdtime)
    : Hold(time, orbit_pos, holdtime) {}

// 析构RMHold
RMHold::~RMHold() = default;
