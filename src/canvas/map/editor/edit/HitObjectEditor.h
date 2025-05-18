#ifndef M_HITOBJECTEDITOR_H
#define M_HITOBJECTEDITOR_H

#include <qevent.h>

#include <memory>
#include <stack>

#include "../../../../mmm/map/MMap.h"
#include "../info/HoverInfo.h"
#include "eventhandler/mousedrag/IMouseDragEventHandler.h"
#include "eventhandler/mousepress/IMousePressEventHandler.h"
#include "eventhandler/mouserelease/IMouseReleaseEventHandler.h"

class MapEditor;

class HitObjectEditor {
   protected:
    // 编辑操作栈
    std::stack<ObjEditOperation> operation_stack;
    // 撤回操作栈
    std::stack<ObjEditOperation> undo_stack;

   public:
    // 构造HitObjectEditor
    HitObjectEditor(MapEditor* meditor_ref);
    // 析构HitObjectEditor
    virtual ~HitObjectEditor();

    // 图编辑器引用
    MapEditor* editor_ref;

    // 鼠标按下时鼠标悬停位置的物件信息快照
    std::shared_ptr<HoverObjectInfo> hover_object_info_shortcut{nullptr};

    // 鼠标按下事件处理器
    std::shared_ptr<IMousePressEventHandler> mpress_handler;

    // 鼠标释放事件处理器
    std::shared_ptr<IMouseReleaseEventHandler> mrelease_handler;

    // 鼠标拖动事件处理器
    std::shared_ptr<IMouseDragEventHandler> mdrag_handler;

    // 正在编辑的原物件
    std::multiset<std::shared_ptr<HitObject>, HitObjectComparator>
        editing_src_objects;

    // 正在编辑的缓存物件
    std::multiset<std::shared_ptr<HitObject>, HitObjectComparator>
        editing_temp_objects;

    // 鼠标按下事件-传递
    virtual void mouse_pressed(QMouseEvent* e);
    // 鼠标释放事件-传递
    virtual void mouse_released(QMouseEvent* e);
    // 鼠标拖动事件-传递
    virtual void mouse_dragged(QMouseEvent* e);

    // 鼠标最近的分拍线的时间
    double nearest_divisor_time();

    // 分析需要删除的物件-自动拆分组合键
    void analyze_src_object();

    // void drag_object()

    // 撤销
    void undo();

    // 重做
    void redo();

    // 复制
    void copy();
    // 粘贴
    void paste();
};

#endif  // M_HITOBJECTEDITOR_H
