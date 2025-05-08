#ifndef M_NOTE_H
#define M_NOTE_H

#include <memory>

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

class NoteInfo : public ObjectInfo {
 public:
  int32_t orbit;
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
  void accept_generate_preview(ObjectGenerator& generator) override;

  // 打印用
  std::string toString() override;

  // 深拷贝
  virtual Note* clone() override;

  virtual NoteInfo* generate_info() override;

  // 是否为相同物件
  virtual bool equals(const Note& other) const;
  virtual bool equals(const std::shared_ptr<Note>& other) const;

  // 比较器使用
  bool lessThan(const HitObject* other) const override;
};

#endif  // M_NOTE_H
