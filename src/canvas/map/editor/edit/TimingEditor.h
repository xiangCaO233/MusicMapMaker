#ifndef M_TIMINGEDITOR_H
#define M_TIMINGEDITOR_H

#include <QMouseEvent>
#include <stack>

#include "../../../../mmm/map/MMap.h"

class MapEditor;

class TimingEditor {
    // 编辑操作栈
    std::stack<TimingEditOperation> operation_stack;
    // 撤回操作栈
    std::stack<TimingEditOperation> undo_stack;

   public:
    // 构造TimingEditor
    TimingEditor(MapEditor* meditor_ref);
    // 析构TimingEditor
    virtual ~TimingEditor();

    // 图编辑器引用
    MapEditor* editor_ref;

    // 是否是剪切操作
    bool is_cut{false};

    // 鼠标按下事件-传递
    virtual void mouse_pressed(QMouseEvent* e);
    virtual void mouse_released(QMouseEvent* e);

    // 鼠标拖动事件-传递
    virtual void mouse_dragged(QMouseEvent* e);

    // 撤销
    void undo();

    // 重做
    void redo();

    // 复制
    void copy();
    // 剪切
    void cut();
    // 粘贴
    void paste();
};

#endif  // M_TIMINGEDITOR_H
