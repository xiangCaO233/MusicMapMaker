#ifndef M_HOLD_H
#define M_HOLD_H

#include "Note.h"

// 长条
class Hold : public Note {
 public:
  // 构造Hold
  Hold(uint32_t time, uint32_t holdtime);
  // 析构Hold
  ~Hold() override;

  // 持续时间
  uint32_t hold_time;
};

#endif  // M_HOLD_H
