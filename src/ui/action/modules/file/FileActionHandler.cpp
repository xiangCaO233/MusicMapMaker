#include <QDebug>
#include <action/modules/file/FileActionHandler.hpp>

// 构造FileActionHandler
FileActionHandler::FileActionHandler(QObject* parent) : QObject(parent) {}

// 析构FileActionHandler
FileActionHandler::~FileActionHandler() {}

// 新建文件action处理
void FileActionHandler::onNewProject() {
    //
    qDebug() << "触发新建项目";
}
void FileActionHandler::onNewFile() {
    //
    qDebug() << "触发新建文件";
}
void FileActionHandler::onOpen() {
    //
    qDebug() << "触发打开文件/项目";
}
void FileActionHandler::onSave() {
    //
    qDebug() << "触发保存";
}
void FileActionHandler::onSaveAs() {
    //
    qDebug() << "触发另存为";
}
void FileActionHandler::onExport() {
    //
    qDebug() << "触发导出";
}
