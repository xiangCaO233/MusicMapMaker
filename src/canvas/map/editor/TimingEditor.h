#ifndef M_TIMINGEDITOR_H
#define M_TIMINGEDITOR_H

#include <memory>
#include <set>
#include <stack>

#include "mmm/timing/Timing.h"

class MapEditor;

class TimingEditOperation {
 public:
  std::multiset<std::shared_ptr<Timing>, TimingComparator> src_timings;
  std::multiset<std::shared_ptr<Timing>, TimingComparator> des_timings;

  TimingEditOperation reverse_clone() const {
    TimingEditOperation reversed_operation;
    reversed_operation.des_timings = src_timings;
    reversed_operation.src_timings = des_timings;
    return reversed_operation;
  }
};

class TimingEditor {
  // 编辑操作栈
  std::stack<TimingEditOperation> operation_stack;
  // 撤回操作栈
  std::stack<TimingEditOperation> undo_stack;

 public:
  // 构造TimingEditor
  TimingEditor(MapEditor* meditor_ref);
  // 析构TimingEditor
  virtual ~TimingEditor();

  // 图编辑器引用
  MapEditor* editor_ref;

  // 撤销
  void undo();

  // 重做
  void redo();
};

#endif  // M_TIMINGEDITOR_H
