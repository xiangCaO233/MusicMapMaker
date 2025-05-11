#ifndef M_IVMPlaceNoteHandler_H
#define M_IVMPlaceNoteHandler_H

#include "../IMousePressEventHandler.h"

class IVMPlaceNoteHandler : public IMousePressEventHandler {
 public:
  // 构造IVMPlaceNoteHandler
  IVMPlaceNoteHandler();
  // 析构IVMPlaceNoteHandler
  ~IVMPlaceNoteHandler() override;

  // 处理事件
  bool handle(HitObjectEditor* oeditor_context, QMouseEvent* e,
              double mouse_time, double mouse_orbit) override;
};

#endif  // M_IVMPlaceNoteHandler_H
