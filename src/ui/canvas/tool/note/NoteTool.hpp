#ifndef MMM_NOTETOOL_HPP
#define MMM_NOTETOOL_HPP

#include <tool/BaseTool.hpp>

class NoteTool : public BaseTool {
   public:
    // 构造NoteTool
    explicit NoteTool(MapCanvas* cvs);
    // 析构NoteTool
    ~NoteTool() override;

   protected:
    // 从Canvas转发过来的事件
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* e) override;
    void keyReleaseEvent(QKeyEvent* e) override;
};

#endif  // MMM_NOTETOOL_HPP
