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

class BaseTool {
   public:
    explicit BaseTool(MapCanvas* cvs) : m_canvas(cvs) {}
    virtual ~BaseTool() = default;

    // 从Canvas转发过来的事件
    virtual void mousePressEvent(QMouseEvent* event) = 0;
    virtual void mouseMoveEvent(QMouseEvent* event) = 0;
    virtual void mouseReleaseEvent(QMouseEvent* event) = 0;
    virtual void keyPressEvent(QKeyEvent* e) = 0;
    virtual void keyReleaseEvent(QKeyEvent* e) = 0;

    // 获取类型
    EditToolType type() const { return editType; }

   protected:
    inline void setType(EditToolType t) { editType = t; }

    inline MapCanvas* canvas() { return m_canvas; }

   private:
    // 工具类型
    EditToolType editType;
    // 获取画布指针
    MapCanvas* m_canvas{nullptr};
};

#endif  // MMM_BASETOOL_HPP
