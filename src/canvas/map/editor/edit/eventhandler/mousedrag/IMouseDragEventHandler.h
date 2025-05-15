#ifndef M_IMOUSEDRAGEVENTHANDLER_H
#define M_IMOUSEDRAGEVENTHANDLER_H

#include <QMouseEvent>
#include <memory>

class HitObjectEditor;

class IMouseDragEventHandler {
   public:
    // 构造IMouseDragEventHandler
    IMouseDragEventHandler() = default;
    // 析构IMouseDragEventHandler
    virtual ~IMouseDragEventHandler() = default;

    // 设置当前处理器的下一个责任
    void set_next_handler(
        std::shared_ptr<IMouseDragEventHandler> next_handler_) {
        next_handler = next_handler_;
    }

    // 处理事件
    virtual bool handle(HitObjectEditor* oeditor_context, QMouseEvent* e,
                        double mouse_time, double mouse_orbit) {
        return false;
    }

   protected:
    // 下一个事件处理器
    std::shared_ptr<IMouseDragEventHandler> next_handler{nullptr};
};

#endif  // M_IMOUSEDRAGEVENTHANDLER_H
