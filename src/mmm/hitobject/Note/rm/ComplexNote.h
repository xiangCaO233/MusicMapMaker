#ifndef M_COMPLEXNOTE_H
#define M_COMPLEXNOTE_H

#include <memory>
#include <vector>

#include "../Note.h"

// 组合键
class ComplexNote : public Note {
 public:
  // 构造ComplexNote
  explicit ComplexNote(uint32_t time);
  // 析构ComplexNote
  ~ComplexNote() override;

  // 所有子物件
  std::vector<std::shared_ptr<Note>> child_notes;

  // 打印用
  std::string toString() override;
};

#endif  // M_COMPLEXNOTE_H
