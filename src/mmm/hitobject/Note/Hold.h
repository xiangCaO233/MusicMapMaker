#ifndef M_HOLD_H
#define M_HOLD_H

#include <memory>

#include "Note.h"

class HoldEnd;

class HoldInfo : public NoteInfo {
 public:
  // 持续时间
  double hold_time;
};

// 长条
class Hold : public Note {
 public:
  // 构造Hold
  Hold(uint32_t time, int32_t orbit_pos, uint32_t holdtime);
  // 析构Hold
  ~Hold() override;

  std::shared_ptr<HoldEnd> hold_end_reference;

  // 持续时间
  uint32_t hold_time;

  // 接收处理
  void accept_generate(ObjectGenerator& generator) override;
  void accept_generate_preview(ObjectGenerator& generator) override;

  // 打印用
  std::string toString() override;

  // 深拷贝
  virtual Hold* clone() override;

  virtual HoldInfo* generate_info() override;

  // 是否为相同物件
  bool equals(const Note& other) const override;
  bool equals(const std::shared_ptr<Note>& other) const override;

  // 比较器使用
  bool lessThan(const HitObject* other) const override;
};

#endif  // M_HOLD_H
