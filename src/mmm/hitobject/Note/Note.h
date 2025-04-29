#ifndef M_NOTE_H
#define M_NOTE_H

#include "../HitObject.h"

class ComplexNote;

enum class NoteType {
  // 另外的滑键和折线
  COMPLEX,
  SLIDE,
  // osu!mania的单键和长条
  HOLD,
  NOTE,
};

enum class ComplexInfo {
  NONE,
  HEAD,
  BODY,
  END,
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

  // 父组合键引用
  ComplexNote* parent_reference{nullptr};

  // 组合键分区
  ComplexInfo compinfo{ComplexInfo::NONE};

  // 接收处理
  void accept_generate(ObjectGenerator& generator) override;

  // 打印用
  std::string toString() override;

  // 比较器使用
  bool lessThan(const HitObject* other) const override;
};

#endif  // M_NOTE_H
