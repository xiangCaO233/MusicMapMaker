#include "OsuNote.h"

#include <cmath>
#include <sstream>
#include <string>

#include "../Note.h"

OsuNote::OsuNote() : Note(0) {
  object_type = HitObjectType::OSUNOTE;
  note_type = NoteType::NOTE;
}
OsuNote::OsuNote(uint32_t time) : Note(time) {
  object_type = HitObjectType::OSUNOTE;
  note_type = NoteType::NOTE;
}

OsuNote::~OsuNote() = default;

// 打印用
std::string OsuNote::toString() { return ""; }

// 从osu描述加载
void OsuNote::from_osu_description(std::vector<std::string>& description,
                                   int32_t orbit_count) {
  // std::string s("");
  // for (const auto& var : description) {
  //  // s.append(var);
  //  XINFO(var);
  //}
  /*
   *长键（仅 osu!mania）
   *长键语法： x,y,开始时间,物件类型,长键音效,结束时间,长键音效组
   *
   *结束时间（整型）： 长键的结束时间，以谱面音频开始为原点，单位是毫秒。
   *x 与长键所在的键位有关。算法为：floor(x * 键位总数 / 512)，并限制在 0 和
   *键位总数 - 1 之间。 *y 不影响长键。默认值为 192，即游戏区域的水平中轴。
   */
  // 位置
  orbit = std::floor(std::stoi(description.at(0)) * orbit_count / 512);

  // 没卵用-om固定192
  // int y = std::stoi(description.at(1));

  // 时间戳
  timestamp = std::stoi(description.at(2));

  note_type = NoteType::NOTE;

  // 音效
  sample = static_cast<NoteSample>(std::stoi(description.at(4)));
  // XINFO("load note:" + s);
  // 单键:剩下的就是音效组参数
  std::string token;
  std::istringstream noteiss(description.at(5));
  std::vector<std::string> last_paras;
  while (std::getline(noteiss, token, ':')) {
    last_paras.push_back(token);
  }

  sample_group.normalSet = static_cast<SampleSet>(std::stoi(last_paras.at(0)));
  sample_group.additionalSet =
      static_cast<NoteSample>(std::stoi(last_paras.at(1)));
  sample_group.sampleSetParameter = std::stoi(last_paras.at(2));
  sample_group.volume = std::stoi(last_paras.at(3));
  if (last_paras.size() == 5) {
    // 有指定key音文件
    sample_group.sampleFile = last_paras.back();
  }
}
