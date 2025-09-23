#ifndef MMM_NOTETOOL_HPP
#define MMM_NOTETOOL_HPP

#include <tool/BaseEditTool.hpp>

class NoteTool : public BaseEditTool {
   public:
    // 构造NoteTool
    using BaseEditTool::BaseEditTool;
    // 析构NoteTool
    ~NoteTool() override;

   protected:
    // 从Canvas转发过来的事件
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

    // 实现基类的纯虚函数，提供 NoteTool 专属的拖拽（编辑/移动）逻辑
    virtual void handleSingleObjectDragStart(
        QMouseEvent* e,
        const std::optional<MeshPartInfo>& hoveredInfo) override;
};

#endif  // MMM_NOTETOOL_HPP
