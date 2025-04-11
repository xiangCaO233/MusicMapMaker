#ifndef M_SLIDE_H
#define M_SLIDE_H

#include "Note.h"

// 滑键
class Slide : public Note {
 public:
  // 构造Slide
  Slide(uint32_t time, int32_t slide_par);
  // 析构Slide
  ~Slide() override;

  // 正右负左,绝对值为滑动的轨道数
  int32_t slide_parameter;
};

#endif  // M_SLIDE_H
