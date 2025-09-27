#ifndef MMM_BASEEDITTOOL_HPP
#define MMM_BASEEDITTOOL_HPP

#include <optional>
#include <tool/BaseTool.hpp>

class BaseEditTool : public BaseTool {
   public:
    // 构造BaseEditTool
    using BaseTool::BaseTool;
    // 析构BaseEditTool
    ~BaseEditTool() override;

   protected:
    // 从Canvas转发过来的事件
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

    // 模板方法：由子类定义如何开始拖拽单个物件
    // NoteTool 会实现为发送带编辑功能的 StartDragCommand
    // SelectTool 会实现为只发送移动命令
    virtual void handleSingleObjectDragStart(
        QMouseEvent* e,
        const std::optional<MeshPartInfo>& hoveredInfo) = 0;  // 纯虚函数
};

#endif  // MMM_BASEEDITTOOL_HPP
