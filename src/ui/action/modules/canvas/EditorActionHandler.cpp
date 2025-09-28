#include <log/colorful-log.h>

#include <QDebug>
#include <action/modules/canvas/EditorActionHandler.hpp>

// 构造EditorActionHandler
EditorActionHandler::EditorActionHandler(QObject* parent) : QObject(parent) {}

// 析构EditorActionHandler
EditorActionHandler::~EditorActionHandler() = default;

void EditorActionHandler::onPause_Resume() {
    //
    emit pause_or_resume_canvas();
    XINFO("触发暂停/恢复");
}

void EditorActionHandler::onSwitch_Handtool() {
    //
    emit switch_handtool();
    XINFO("触发切换到Hand工具");
}

void EditorActionHandler::onSwitch_Notetool() {
    //
    emit switch_notetool();
    XINFO("触发切换到Note工具");
}

void EditorActionHandler::onCancel() {
    //
    XINFO("触发取消");
}

void EditorActionHandler::onSelectPage() {
    //
    XINFO("触发选择页");
}
void EditorActionHandler::onSelectAll() {
    //
    XINFO("触发全选");
}
void EditorActionHandler::onCut() {
    //
    emit cut();
    XINFO("触发剪切");
}
void EditorActionHandler::onCopy() {
    //
    emit copy();
    XINFO("触发拷贝");
}
void EditorActionHandler::onPaste() {
    //
    emit paste();
    XINFO("触发粘贴");
}
void EditorActionHandler::onDelete() {
    //
    emit delete_signal();
    XINFO("触发删除");
}
void EditorActionHandler::onUndo() {
    //
    emit undo();
    XINFO("触发撤销");
}
void EditorActionHandler::onRedo() {
    //
    emit redo();
    XINFO("触发重做");
}

void EditorActionHandler::onFind() {
    //
    XINFO("触发查找");
}
