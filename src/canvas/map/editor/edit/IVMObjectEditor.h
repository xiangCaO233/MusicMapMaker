#ifndef M_IVMOBJECTEDITOR_H
#define M_IVMOBJECTEDITOR_H

#include "editor/edit/HitObjectEditor.h"

class IVMObjectEditor : public HitObjectEditor {
 public:
  // 构造IVMObjectEditor
  IVMObjectEditor(MapEditor* meditor_ref);
  // 析构IVMObjectEditor
  ~IVMObjectEditor() override;

  // 面条编辑模式
  bool long_note_edit_mode{false};

  // 鼠标按下事件-传递
  void mouse_pressed(QMouseEvent* e) override;

  // 鼠标拖动事件-传递
  void mouse_dragged(QMouseEvent* e) override;
};

#endif  // M_IVMOBJECTEDITOR_H
