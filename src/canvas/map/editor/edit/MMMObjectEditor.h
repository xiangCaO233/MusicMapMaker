#ifndef M_MMMOBJECTEDITOR_H
#define M_MMMOBJECTEDITOR_H

#include "HitObjectEditor.h"

class MMMObjectEditor : public HitObjectEditor {
   public:
    // 构造MMMObjectEditor
    MMMObjectEditor(MapEditor* meditor_ref);
    // 析构MMMObjectEditor
    ~MMMObjectEditor() override;

    // 鼠标按下事件-传递
    void mouse_pressed(QMouseEvent* e) override;
    // 鼠标释放事件-传递
    void mouse_released(QMouseEvent* e) override;
    // 鼠标拖动事件-传递
    void mouse_dragged(QMouseEvent* e) override;
};

#endif  // M_MMMOBJECTEDITOR_H
