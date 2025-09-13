#ifndef MMM_NOTETOOL_HPP
#define MMM_NOTETOOL_HPP

#include <tool/BaseTool.hpp>

class NoteTool : public BaseTool {
   public:
    // 构造NoteTool
    using BaseTool::BaseTool;
    // 析构NoteTool
    ~NoteTool() override;

   protected:
    // 从Canvas转发过来的事件
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    void keyReleaseEvent(QKeyEvent* e) override;
};

#endif  // MMM_NOTETOOL_HPP
