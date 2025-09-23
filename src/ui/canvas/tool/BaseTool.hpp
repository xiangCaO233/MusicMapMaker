#ifndef MMM_BASETOOL_HPP
#define MMM_BASETOOL_HPP

#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <tool/ThreadSafeQueue.hpp>
#include <tool/ToolInteractionState.hpp>
#include <tool/command/ToolCommand.hpp>

enum class EditToolType {
    // 无操作的hand工具只有基本操作和拖动进度的功能
    HAND,
    // 放置物件工具-同时可以删除物件
    NOTE,
    // 选择工具
    SELECT,
};

class MapCanvas;
class GLDirectPainter;
class ToolSystem;

class BaseTool {
   public:
    explicit BaseTool(MapCanvas* cvs, ToolSystem* const tool_system,
                      ThreadSafeQueue<ToolCommand>* const tool_cmdq,
                      ToolInteractionState* const tool_interaction_state)
        : m_canvas(cvs),
          toolSystem(tool_system),
          toolCommandQueue(tool_cmdq),
          toolInteractionState(tool_interaction_state) {}
    virtual ~BaseTool() = default;

    // 从Canvas转发过来的事件
    virtual void mousePressEvent(QMouseEvent* e);
    virtual void mouseMoveEvent(QMouseEvent* e);
    virtual void mouseReleaseEvent(QMouseEvent* e);
    virtual void wheelEvent(QWheelEvent* e);

    virtual void keyPressEvent(QKeyEvent* e);
    virtual void keyReleaseEvent(QKeyEvent* e);

   protected:
    // 访问画布
    inline MapCanvas* canvas() { return m_canvas; }

    // 访问工具系统
    inline ToolSystem* tool_system() { return toolSystem; }

    // 访问工具指令队列
    inline ThreadSafeQueue<ToolCommand>* tool_command_queue() {
        return toolCommandQueue;
    }

    // 访问交互管理器
    inline ToolInteractionState* tool_interaction_state() {
        return toolInteractionState;
    }

   private:
    // 画布指针
    MapCanvas* m_canvas{nullptr};
    // 工具系统指针
    ToolSystem* const toolSystem;
    // 工具指令队列指针
    ThreadSafeQueue<ToolCommand>* const toolCommandQueue;
    // 工具交互状态管理器指针
    ToolInteractionState* const toolInteractionState;
};

#endif  // MMM_BASETOOL_HPP
