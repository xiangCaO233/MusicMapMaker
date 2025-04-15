#ifndef M_OSUHOLD_H
#define M_OSUHOLD_H

#include <memory>

#include "../Hold.h"
#include "../HoldEnd.h"
#include "OsuNote.h"

class OsuHold : public Hold {
 public:
  // 构造OsuHold
  OsuHold(uint32_t time, uint32_t holdtime);
  OsuHold();
  // 析构OsuHold
  ~OsuHold() override;

  // note采样
  NoteSample sample;

  // 物件音效组
  NoteSampleGroup sample_group;

  // 打印用
  std::string toString() override;

  // 从osu描述加载
  void from_osu_description(std::vector<std::string>& description,
                            int32_t orbit_count);
};

// 面尾
class OsuHoldEnd : public HoldEnd {
 public:
  // 构造OsuHoldEnd
  explicit OsuHoldEnd(const std::shared_ptr<OsuHold>& ohold);
  // 析构OsuHoldEnd
  ~OsuHoldEnd() override;

  // 对应面条物件引用
  std::shared_ptr<OsuHold> reference;

  // 打印用
  std::string toString() override;
};

#endif  // M_OSUHOLD_H
