#include "OsuNote.h"

#include "src/mmm/hitobject/Note/Note.h"

OsuNote::OsuNote(uint32_t time) : Note(time) {}

OsuNote::~OsuNote() = default;

// 打印用
std::string OsuNote::toString() { return ""; }

// 从osu描述加载
void OsuNote::from_osu_description(const std::string &description) {}
