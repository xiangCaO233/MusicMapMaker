#include <log/colorful-log.h>

#include <QDebug>
#include <action/modules/file/FileActionHandler.hpp>
#include <util/mutil.hpp>

// 构造FileActionHandler
FileActionHandler::FileActionHandler(QObject* parent) : QObject(parent) {}

// 析构FileActionHandler
FileActionHandler::~FileActionHandler() {}

// 新建文件action处理
void FileActionHandler::onNewProject() {
    //
    XINFO("触发新建项目");
}
void FileActionHandler::onNewFile() {
    //
    XINFO("触发新建文件");
    emit newFile();
}

void FileActionHandler::onOpen() {
    //
    XINFO("触发打开文件");
    emit open();
}

void FileActionHandler::onOpenDirectory() {
    XINFO("触发打开文件夹");
    emit open_directory();
}
void FileActionHandler::onSave() {
    //
    XINFO("触发保存");
    emit save();
}
void FileActionHandler::onSaveAs() {
    //
    XINFO("触发另存为")
    emit save_as();
}
void FileActionHandler::onExport() {
    //
    XINFO("触发导出");
    emit export_as();
}
