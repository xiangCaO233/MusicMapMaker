#ifndef M_HITOBJECTEDITOR_H
#define M_HITOBJECTEDITOR_H

#include <qevent.h>

#include <memory>
#include <stack>

#include "../../../../mmm/map/MMap.h"

class MapEditor;

class HitObjectEditor {
 protected:
  // 编辑操作栈
  std::stack<ObjEditOperation> operation_stack;
  // 撤回操作栈
  std::stack<ObjEditOperation> undo_stack;

 public:
  // 构造HitObjectEditor
  HitObjectEditor(MapEditor* meditor_ref);
  // 析构HitObjectEditor
  virtual ~HitObjectEditor();

  // 图编辑器引用
  MapEditor* editor_ref;

  // 正在编辑的原物件
  std::multiset<std::shared_ptr<HitObject>, HitObjectComparator>
      editing_src_objects;

  // 正在编辑的缓存物件
  std::multiset<std::shared_ptr<HitObject>, HitObjectComparator>
      editing_temp_objects;

  // 鼠标按下事件-传递
  virtual void mouse_pressed(QMouseEvent* e);
  // 鼠标释放事件-传递
  virtual void mouse_released(QMouseEvent* e);
  // 鼠标拖动事件-传递
  virtual void mouse_dragged(QMouseEvent* e);

  // 鼠标最近的分拍线的时间
  double nearest_divisor_time();

  // void drag_object()

  // 撤销
  void undo();

  // 重做
  void redo();
};

#endif  // M_HITOBJECTEDITOR_H
