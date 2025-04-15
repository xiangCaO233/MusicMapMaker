#ifndef M_HOLD_H
#define M_HOLD_H

#include <memory>

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

  // 打印用
  std::string toString() override;
};

// 面尾
class HoldEnd : public HitObject {
 public:
  // 构造HoldEnd
  HoldEnd(const std::shared_ptr<Hold>& hold);
  // 析构HoldEnd
  ~HoldEnd() override;

  // 对应面条物件引用
  std::shared_ptr<Hold> reference;

  // 打印用
  std::string toString() override;
};

#endif  // M_HOLD_H
