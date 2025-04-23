#ifndef M_HOLD_H
#define M_HOLD_H

#include <memory>

#include "Note.h"

class HoldEnd;

// 长条
class Hold : public Note {
 public:
  // 构造Hold
  Hold(uint32_t time, uint32_t holdtime, int32_t orbit_pos);
  // 析构Hold
  ~Hold() override;

  HoldEnd* hold_end_reference;

  // 持续时间
  uint32_t hold_time;

  // 打印用
  std::string toString() override;
};

#endif  // M_HOLD_H
