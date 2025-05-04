#ifndef M_HITOBJECTEDITOR_H
#define M_HITOBJECTEDITOR_H

#include <memory>
#include <set>
#include <stack>

#include "../../../mmm/hitobject/HitObject.h"

class ObjEditOperation {
 public:
  std::multiset<std::shared_ptr<HitObject>, HitObjectComparator> src_objects;
  std::multiset<std::shared_ptr<HitObject>, HitObjectComparator> des_objects;

  ObjEditOperation reverse_clone() const {
    ObjEditOperation reversed_operation;
    reversed_operation.des_objects = src_objects;
    reversed_operation.src_objects = des_objects;
  }
};

class HitObjectEditor {
  // 编辑操作栈
  std::stack<ObjEditOperation> operation_stack;
  // 撤回操作栈
  std::stack<ObjEditOperation> undo_stack;

 public:
  // 构造HitObjectEditor
  HitObjectEditor();
  // 析构HitObjectEditor
  virtual ~HitObjectEditor();

  // 撤销
  void undo();

  // 重做
  void redo();
};

#endif  // M_HITOBJECTEDITOR_H
