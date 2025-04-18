#ifndef M_NOTE_H
#define M_NOTE_H

#include "../HitObject.h"

class Note : public HitObject {
 public:
  // 构造Note
  explicit Note(uint32_t time);
  // 析构Note
  ~Note() override;

  // 物件类型
  NoteType type;

  // 物件所处轨道
  int32_t orbit;

  // 打印用
  std::string toString() override;
};

#endif  // M_NOTE_H
