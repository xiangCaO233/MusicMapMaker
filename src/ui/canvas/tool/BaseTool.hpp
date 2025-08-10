#ifndef MMM_BASETOOL_HPP
#define MMM_BASETOOL_HPP

#include <QMouseEvent>

class GLCanvas;
class MPainter;

class BaseTool {
   public:
    virtual ~BaseTool() = default;

    // 工具被激活或失活时由Canvas调用
    virtual void activate(GLCanvas* canvas) { m_canvas = canvas; }
    virtual void deactivate() {}

    // 从GLCanvas转发过来的事件
    virtual void mousePressEvent(QMouseEvent* event) = 0;
    virtual void mouseMoveEvent(QMouseEvent* event) = 0;
    virtual void mouseReleaseEvent(QMouseEvent* event) = 0;

    // 由GLCanvas的paintGL()调用，绘制交互过程中的临时图形
    virtual void drawFeedback(MPainter* painter) = 0;

   protected:
    inline GLCanvas* canvas() { return m_canvas; }

   private:
    GLCanvas* m_canvas{nullptr};
};

#endif  // MMM_BASETOOL_HPP
