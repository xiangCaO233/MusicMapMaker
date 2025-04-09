#ifndef M_NOTE_H
#define M_NOTE_H

#include <string>

#include "../HitObject.h"
/*
类型
物件类型参数是一个 8 位整数，每一位都有特殊的含义。

位次序	含义
0	将物件标记为圆圈
1	将物件标记为滑条
2	标记新 Combo 起始物件
3	将物件标记为转盘
4, 5, 6	一个 3 位整数，指定要跳过多少 Combo 颜色
（即“跳过连击色 (Colourhax)”）。
仅在物件位于新 Combo 开始时才相关。 7	将物件标记为 osu!mania 长按音符
*/

// note采样
/*
音效
音效数据用于标记物件的打击音效：

位次序	含义
0	Normal（普通）
2	Whistle（口哨）
4	Finish（镲）
8	Clap（鼓掌）
如果没有设置打击音效，则默认为 Normal。

除了 osu!mania 模式之外，在其他模式中游玩，无论皮肤设置的
LayeredHitSounds（总是播放 Normal 音效） 设为何种状态，它都将默认强制启用。
*/
enum class NoteSample : uint32_t {
  NORMAL = 0,
  WHISTLE = 1,
  FINISH = 2,
  CLAP = 3,
};
/* osuwiki
自定义音效组
使用打击音效组数据能更细化地自定义物件的音效。如果没有设置特殊参数，则默认为
0:0:0:0:。

打击音效组语法：普通音效组:附加音效组:参数:音量:文件名

普通音效组（整型）： Normal 音效的音效组
附加音效组（整型）： Whistle、Finish、Clap 音效的音效组
音效组编号（整型）：
用于区分不同音效组的编号。如果是0，则表示这个打击物件的音效组
，沿用控制该物件时间点的音效组设定。
音量（整型）：音效组的音量。如果是0，
则表示这个打击物件的音量，沿用控制该物件时间点的音量设定。
文件名（字符串）：
自定义附加音效的文件名或相对路径。

普通音效组和附加音效组可以设为以下的这些值：
0：未设置自定义音效组
对于普通打击音效，音效组由控制该物件的时间点决定。
对于附加打击音效， 音效组由普通打击音效组决定。
1：Normal 组
2：Soft 组
3：Drum 组
以上所有设置（不包括音量设置）共同决定了击打物件时应当播放何种音效。
这个音效文件名格式为：<音效组名称>-hit<音效名称><音效组编号>.wav，其中：

音效组名称可设为 normal、soft、drum，这确定了普通打击音效或附加打击音效的音效组
音效名称可设为 normal、whistle、finish、clap
音效组编号与上述段落的音效组编号相同。如果值为 0 或 1，则可忽略不写
当加载谱面时，游戏会自动按从上到下的优先级加载对应名称的音效。

当音效组编号不设为 0 时，加载谱面内的音效
当无法在谱面内找到该音效组编号对应的文件时，加载玩家皮肤内的音效
当无法在玩家皮肤内找到该音效组编号对应的文件时，加载 osu! 默认的音效
当填写了文件名，此时游戏会将这个文件替换掉物件默认的附加打击音效。
*/
enum class SampleSet : uint32_t {
  UNDEFINE = 0,
  NORMAL = 1,
  SOFT = 2,
  DRUM = 3,
};

// 物件音效组
struct NoteSampleGroup {
  // osu!standard模式且LayeredHitSounds启用时
  // 会与设置的附加音效组混合
  // 普通音效组
  //
  // 对于普通打击音效，音效组由控制该物件的时间点决定。
  // 就是timing
  SampleSet normalSet;
  // 对于附加打击音效， 音效组由普通打击音效组决定。
  // 附加音效组
  NoteSample additionalSet;
  // 音效组编号（整型）：
  // 用于区分不同音效组的编号。如果是0，则表示这个打击物件的音效组
  // ，沿用控制该物件时间点的音效组设定。
  uint32_t sampleSetParameter;
  // 音量（整型）：音效组的音量。如果是0，
  // 则表示这个打击物件的音量，沿用控制该物件时间点的音量设定。
  uint32_t volume;
  // 当音效组编号不设为 0 时，加载谱面内的音效
  // 当无法在谱面内找到该音效组编号对应的文件时，加载玩家皮肤内的音效
  // 当无法在玩家皮肤内找到该音效组编号对应的文件时，加载 osu! 默认的音效
  // 当填写了文件名，此时游戏会将这个文件替换掉物件默认的附加打击音效
  std::string sampleFile;
  // 最终这个音效文件名格式为：<音效组名称>-hit<音效名称><音效组编号>.wav，
  // 其中：
  // 自定义音效文件>谱面文件夹内音效文件>玩家皮肤内音效文件>osu默认音效文件
};

class Note : public HitObject {
 public:
  // 构造Note
  explicit Note(uint32_t time);
  // 析构Note
  ~Note() override;

  // 物件类型
  HitObjectType type;

  // note采样
  NoteSample sample;

  // 物件音效组
  NoteSampleGroup sample_group;
};

#endif  // M_NOTE_H
