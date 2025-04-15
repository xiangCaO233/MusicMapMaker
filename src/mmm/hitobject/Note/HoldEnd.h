#ifndef M_HOLDEND_H
#define M_HOLDEND_H

#include <memory>

#include "Hold.h"

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

#endif  // M_HOLDEND_H
