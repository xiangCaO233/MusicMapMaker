#ifndef M_IMOUSERELEASEEVENTHANDLER_H
#define M_IMOUSERELEASEEVENTHANDLER_H

#include <QMouseEvent>
#include <memory>

class HitObjectEditor;

class IMouseReleaseEventHandler {
   public:
    IMouseReleaseEventHandler() = default;
    // 析构IMouseReleaseEventHandler
    virtual ~IMouseReleaseEventHandler() = default;

    // 设置当前处理器的下一个责任
    void set_next_handler(
        std::shared_ptr<IMouseReleaseEventHandler> next_handler_) {
        next_handler = next_handler_;
    }

    // 处理事件
    virtual bool handle(HitObjectEditor* oeditor_context, QMouseEvent* e,
                        double mouse_time, double mouse_orbit) {
        return false;
    }

   protected:
    // 下一个事件处理器
    std::shared_ptr<IMouseReleaseEventHandler> next_handler{nullptr};
};

#endif  // M_IMOUSERELEASEEVENTHANDLER_H
