#ifndef M_SLIDE_H
#define M_SLIDE_H

#include <memory>

#include "../../HitObject.h"
#include "../Note.h"

class SlideEnd;

// 滑键
class Slide : public Note {
 public:
  // 构造Slide
  Slide(uint32_t time, int32_t orbit_pos, int32_t slide_par);

  // 析构Slide
  ~Slide() override;

  // 滑键尾引用
  std::shared_ptr<SlideEnd> slide_end_reference;

  // 正右负左,绝对值为滑动的轨道数
  int32_t slide_parameter;

  // 接收处理
  void accept_generate(ObjectGenerator& generator) override;
  void accept_generate_preview(ObjectGenerator& generator) override;

  // 打印用
  std::string toString() override;

  // 比较器使用
  bool lessThan(const HitObject* other) const override;
};

// 滑键尾
class SlideEnd : public HitObject {
 public:
  // 构造SlideEnd
  SlideEnd(const std::shared_ptr<Slide>& head);
  // 析构SlideEnd
  ~SlideEnd() override;

  // 滑键尾所处轨道
  int32_t endorbit;

  // 对应的滑键引用
  Slide* reference;

  // 接收处理
  void accept_generate(ObjectGenerator& generator) override;
  void accept_generate_preview(ObjectGenerator& generator) override;

  // 打印用
  std::string toString() override;

  // 比较器使用
  bool lessThan(const HitObject* other) const override;
};
#endif  // M_SLIDE_H
