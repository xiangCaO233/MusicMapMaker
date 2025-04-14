#include "OsuHold.h"

OsuHold::OsuHold(uint32_t time, uint32_t holdtime)
    : OsuNote(time), hold_time(holdtime) {}

OsuHold::~OsuHold() {}

// 打印用
std::string OsuHold::toString() { return ""; }

// 从osu描述加载
void OsuHold::from_osu_description(const std::string &description) {}
