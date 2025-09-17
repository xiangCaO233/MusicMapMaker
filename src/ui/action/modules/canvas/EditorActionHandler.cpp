#include <QDebug>
#include <action/modules/canvas/EditorActionHandler.hpp>

// 构造EditorActionHandler
EditorActionHandler::EditorActionHandler(QObject* parent) : QObject(parent) {}

// 析构EditorActionHandler
EditorActionHandler::~EditorActionHandler() = default;

void EditorActionHandler::onPause_Resume() {
    //
    emit pause_or_resume_canvas();
    qDebug() << "触发暂停/恢复";
}
void EditorActionHandler::onCancel() {
    //
    qDebug() << "触发取消";
}

void EditorActionHandler::onSelectPage() {
    //
    qDebug() << "触发选择页";
}
void EditorActionHandler::onSelectAll() {
    //
    qDebug() << "触发全选";
}
void EditorActionHandler::onCut() {
    //
    qDebug() << "触发剪切";
}
void EditorActionHandler::onCopy() {
    //
    qDebug() << "触发拷贝";
}
void EditorActionHandler::onPaste() {
    //
    qDebug() << "触发粘贴";
}
void EditorActionHandler::onDelete() {
    //
    qDebug() << "触发删除";
}
void EditorActionHandler::onUndo() {
    //
    qDebug() << "触发撤销";
}
void EditorActionHandler::onRedo() {
    //
    qDebug() << "触发重做";
}

void EditorActionHandler::onFind() {
    //
    qDebug() << "触发查找";
}
