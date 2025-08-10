#ifndef MMM_SELECTTOOL_HPP
#define MMM_SELECTTOOL_HPP

#include <tool/BaseTool.hpp>

class SelectTool : public BaseTool {
   public:
    // 构造SelectTool
    SelectTool();
    // 析构SelectTool
    ~SelectTool() override;

   protected:
    // 从GLCanvas转发过来的事件
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

    // 由GLCanvas的paintGL()调用，绘制交互过程中的临时图形
    void drawFeedback(MPainter* painter) override;
};
#endif  // MMM_SELECTTOOL_HPP
