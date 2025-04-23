#ifndef M_NOTE_H
#define M_NOTE_H

#include "../HitObject.h"

enum class NoteType : uint8_t {
  // osu!mania的单键和长条
  NOTE = 0b00000001,
  HOLD = 0b01000000,
  // 另外的滑键和折线
  SLIDE = 0b00000010,
  COMPLEX = 0b00000100,
};

class Note : public HitObject {
 public:
  // 构造Note
  explicit Note(uint32_t time, int32_t orbit_pos);
  // 析构Note
  ~Note() override;

  // 物件类型
  NoteType note_type;

  // 物件所处轨道
  int32_t orbit;

  // 打印用
  std::string toString() override;
};

#endif  // M_NOTE_H
