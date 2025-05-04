#ifndef M_OSU_TIMING_H
#define M_OSU_TIMING_H

/*
* osu-wiki
*时间点
*每个时间点都会控制谱面的一部分，又常称为“时间轴区间”。.osu
格式要求这些时间点在记录时，需要按时间顺序排序。
*
*时间语法：时间,拍长,节拍,音效组,音效参数,音量,是否为非继承时间点（红线）,效果
*
*例子
*10000,333.33,4,0,0,100,1,1
*12000,-25,4,3,0,100,0,1
*10 秒处的第一个时间点为非继承时间点（红线），并包含以下的设置：
*
*BPM 为 180（1 / 333.33 * 1000 * 60）
*拍子记号为 4/4
*沿用谱面的默认音效组
*使用 osu! 的默认音效
*音量 100%
*开启 Kiai 时间
*12 秒处的第二个时间点为继承时间点（绿线），改变滑条速度为 4
*倍，并将这一段的音效组切换成 drum 音效组。
*/

#include <string>
#include <vector>

#include "../../SampleSet.h"
#include "../Timing.h"

class OsuTiming : public Timing {
 public:
  // 构造OsuTiming
  OsuTiming();
  // 析构OsuTiming
  ~OsuTiming() override;

  // osu-时间点语法：时间,拍长,节拍,音效组,音效参数,音量,是否为非继承时间点（红线）,效果

  // 节拍（整型）：
  // 一小节中的拍子数量。继承时间点（绿线）的这个值无效果。
  int32_t beat{4};

  // 该时间点的音效组
  // 音效组（整型）：
  // 物件使用的默认音效组（0 = 谱面默认设置（SampleSet），1 = normal，2 =
  // soft，3 = *drum）。
  SampleSet sample_set{SampleSet::NORMAL};

  // 音效参数（整型）： 物件使用的自定义音效参数。 0 表示使用 osu!
  // 默认的音效。
  int32_t sample_parameter{0};

  // *音量（整型）： 击打物件的音量（0 - 100）。
  int32_t volume{100};

  // *是否为非继承时间点（红线）（布尔值）： 字面意思。
  bool is_inherit_timing{false};

  // 效果（整型）： 一位影响时间点特殊效果的参数。参见：效果部分。
  // 效果
  // 时间点可以通过在效果（整型）参数中，通过修改参数值为 1 或者
  // 8（二进制下，是第 0 位和第 3 位），来开启两种特殊效果。
  // 1（二进制的第 0 位）0b00000001：是否开启 Kiai 时间 -----------没卵用
  // 8（二进制的第 3 位）0b00001000：是否在 osu!taiko 和 osu!mania
  // 模式下，忽略红线的第一条小节线显示
  // 若想同时开启两种效果，则可填 9（1 + 8
  // =9）0b00001001。其余的二进制位暂不使用。
  int8_t effect{0b00000000};

  // 转换为osu的字符串
  std::string to_osu_description();

  // 从osu的字符串读取
  void from_osu_description(std::vector<std::string>& description);
};

#endif  // M_OSU_TIMING_H
