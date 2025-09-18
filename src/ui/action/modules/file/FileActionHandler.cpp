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
    qDebug() << "触发新建项目";
}
void FileActionHandler::onNewFile() {
    //
    qDebug() << "触发新建文件";
}
void FileActionHandler::onOpen() {
    //
    qDebug() << "触发打开文件";
    // 使用文件夹选择器选择项目的目录
    auto file = mutil::getOpenFile(nullptr, tr("select file"),
                                   {{tr("Audio File"), ".ogg .mp3 .wav"},
                                    {tr("Map File"), ".imd .mmm .osu .mc"}},
                                   QDir::homePath());
    if (!file.isEmpty()) {
        auto fpath = file.toStdString();
        // 打开项目
        emit open(fpath);
    } else {
        // 取消打开
        qDebug() << "取消打开文件";
    }
}
void FileActionHandler::onOpenDirectory() {
    qDebug() << "触发打开文件夹";
    // 使用文件夹选择器选择项目的目录
    auto dir = mutil::getDirectory(nullptr, tr("select project directory"),
                                   QDir::homePath());
    if (!dir.isEmpty()) {
        auto ppath = dir.toStdString();
        // 打开项目
        emit open_directory(ppath);
    } else {
        // 取消打开
        qDebug() << "取消打开文件夹";
    }
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
