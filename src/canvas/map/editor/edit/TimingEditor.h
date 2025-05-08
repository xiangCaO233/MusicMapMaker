#ifndef M_TIMINGEDITOR_H
#define M_TIMINGEDITOR_H

#include <QMouseEvent>
#include <stack>

#include "../../../../mmm/map/MMap.h"

class MapEditor;

class TimingEditor {
  // 编辑操作栈
  static std::stack<TimingEditOperation> operation_stack;
  // 撤回操作栈
  static std::stack<TimingEditOperation> undo_stack;

 public:
  // 构造TimingEditor
  TimingEditor(MapEditor* meditor_ref);
  // 析构TimingEditor
  virtual ~TimingEditor();

  // 图编辑器引用
  MapEditor* editor_ref;

  // 鼠标按下事件-传递
  virtual void mouse_pressed(QMouseEvent* e);
  virtual void mouse_released(QMouseEvent* e);

  // 鼠标拖动事件-传递
  virtual void mouse_dragged(QMouseEvent* e);

  // 撤销
  static void undo();

  // 重做
  static void redo();
};

#endif  // M_TIMINGEDITOR_H
