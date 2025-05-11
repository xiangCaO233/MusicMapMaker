#ifndef M_IMOUSEPRESS_HANDLER_H
#define M_IMOUSEPRESS_HANDLER_H

#include <QMouseEvent>
#include <memory>

class HitObjectEditor;

class IMousePressEventHandler {
   public:
    IMousePressEventHandler() = default;
    // 析构IMousePressEventHandler
    virtual ~IMousePressEventHandler() = default;

    // 设置当前处理器的下一个责任
    void set_next_handler(
        std::shared_ptr<IMousePressEventHandler> next_handler_) {
        next_handler = next_handler_;
    };

    // 处理事件
    virtual bool handle(HitObjectEditor* oeditor_context, QMouseEvent* e,
                        double mouse_time, double mouse_orbit) = 0;

   protected:
    // 下一个事件处理器
    std::shared_ptr<IMousePressEventHandler> next_handler{nullptr};
};

#endif  // M_IMOUSEPRESS_HANDLER_H
