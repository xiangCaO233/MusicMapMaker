#ifndef M_HOLD_H
#define M_HOLD_H

#include <memory>

#include "Note.h"

class HoldEnd;

// 长条
class Hold : public Note {
 public:
  // 构造Hold
  Hold(uint32_t time, int32_t orbit_pos, uint32_t holdtime);
  // 析构Hold
  ~Hold() override;

  HoldEnd* hold_end_reference;

  // 持续时间
  uint32_t hold_time;

  // 打印用
  std::string toString() override;

  // 比较器使用
  bool lessThan(const HitObject* other) const override;
};

#endif  // M_HOLD_H
