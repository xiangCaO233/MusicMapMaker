#ifndef MMM_SELECTTOOL_HPP
#define MMM_SELECTTOOL_HPP

#include <tool/BaseEditTool.hpp>

class HandTool : public BaseEditTool {
   public:
    // 构造SelectTool
    using BaseEditTool::BaseEditTool;
    // 析构SelectTool
    ~HandTool() override;

   protected:
    // 从Canvas转发过来的事件
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

    // 实现基类的纯虚函数，提供 SelectTool 的强制移动逻辑
    virtual void handleSingleObjectDragStart(
        QMouseEvent* e,
        const std::optional<MeshPartInfo>& hoveredInfo) override;
};
#endif  // MMM_SELECTTOOL_HPP
