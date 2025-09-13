#ifndef MMM_SELECTTOOL_HPP
#define MMM_SELECTTOOL_HPP

#include <tool/BaseTool.hpp>

class SelectTool : public BaseTool {
   public:
    // 构造SelectTool
    using BaseTool::BaseTool;
    // 析构SelectTool
    ~SelectTool() override;

   protected:
    // 从Canvas转发过来的事件
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    void keyReleaseEvent(QKeyEvent* e) override;
};
#endif  // MMM_SELECTTOOL_HPP
