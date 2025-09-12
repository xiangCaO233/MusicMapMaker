#ifndef MMM_BASETOOL_HPP
#define MMM_BASETOOL_HPP

#include <QKeyEvent>
#include <QMouseEvent>

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
    explicit BaseTool(MapCanvas* cvs, const ToolSystem* tool_system)
        : m_canvas(cvs), toolSystem(tool_system) {}
    virtual ~BaseTool() = default;

    // 从Canvas转发过来的事件
    virtual void mousePressEvent(QMouseEvent* event) = 0;
    virtual void mouseMoveEvent(QMouseEvent* event) = 0;
    virtual void mouseReleaseEvent(QMouseEvent* event) = 0;
    virtual void keyPressEvent(QKeyEvent* e) = 0;
    virtual void keyReleaseEvent(QKeyEvent* e) = 0;

   protected:
    // 访问画布
    inline MapCanvas* canvas() const { return m_canvas; }

    // 访问工具系统
    inline const ToolSystem* tool_system() const { return toolSystem; }

   private:
    // 画布指针
    MapCanvas* m_canvas{nullptr};
    // 工具系统指针
    const ToolSystem* toolSystem;
};

#endif  // MMM_BASETOOL_HPP
