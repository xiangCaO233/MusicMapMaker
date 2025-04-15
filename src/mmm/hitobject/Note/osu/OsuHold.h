#ifndef M_OSUHOLD_H
#define M_OSUHOLD_H

#include <memory>

#include "../../HitObject.h"
#include "OsuNote.h"

class OsuHold : public OsuNote {
 public:
  // 构造OsuHold
  OsuHold(uint32_t time, uint32_t holdtime);
  OsuHold();
  // 析构OsuHold
  ~OsuHold() override;

  // 持续时间
  uint32_t hold_time;

  // 打印用
  std::string toString() override;

  // 从osu描述加载
  void from_osu_description(std::vector<std::string>& description,
                            int32_t orbit_count) override;
};

// 面尾
class OsuHoldEnd : public HitObject {
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
