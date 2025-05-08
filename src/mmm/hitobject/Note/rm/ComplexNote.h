#ifndef M_COMPLEXNOTE_H
#define M_COMPLEXNOTE_H

#include <memory>
#include <vector>

#include "../Note.h"

// 组合键
class ComplexNote : public Note {
 public:
  // 构造ComplexNote
  explicit ComplexNote(uint32_t time, int32_t orbit_pos);
  // 析构ComplexNote
  ~ComplexNote() override;

  // 所有子物件
  std::vector<std::shared_ptr<Note>> child_notes;

  // 打印用
  std::string toString() override;

  // 深拷贝
  virtual ComplexNote* clone() override;
};

#endif  // M_COMPLEXNOTE_H
