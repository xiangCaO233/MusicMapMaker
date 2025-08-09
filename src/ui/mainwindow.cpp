#include "mainwindow.h"

#include <audio/track/trackmanager.h>
#include <project/projectmanager.h>
#include <ui_mainwindow.h>

#include <canvas/map/MapCanvas.hpp>
#include <mmm/map/MMap.hpp>
#include <mmm/obj/Note.hpp>
#include <mmm/project/MProject.hpp>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    qRegisterMetaType<std::string>("std::string");
    // 注册 shared_ptr<MMap> 类型
    qRegisterMetaType<MMap*>("MMap*");

    editor = new MapEditor();
    projectmanager = new ProjectManager();
    projectmanager->bind_canvas(editor->canvas());
    trackmanager = new TrackManager();
    projectmanager->bind_trackmgr(trackmanager);

    // 捕获ui指针
    auto capui = ui;

    connect(editor, &MapEditor::close_signal,
            [capui]() { capui->actionEditor->setChecked(false); });
    connect(trackmanager, &TrackManager::close_signal,
            [capui]() { capui->actionTrack_Manager->setChecked(false); });
    connect(projectmanager, &ProjectManager::close_signal,
            [capui]() { capui->actionProject_Manager->setChecked(false); });

    // MMap map(
    //     "/Users/2333xiang/Downloads/Juggernaut. - Antler/Juggernaut. - Antler
    //     "
    //     "(xiang_233) [NOInsane].osu");
    // auto notes = map.note_set().get_all_notes_ordered();
    // for (const auto& handle : notes) {
    //     qDebug() << map.note_set().get_note(handle)->toString();
    // }
}

MainWindow::~MainWindow() {
    // 先清理项目(需要使用editor的gl上下文移除纹理)
    delete projectmanager;
    delete editor;
    delete trackmanager;
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent* e) {
    editor->close();
    trackmanager->close();
    projectmanager->close();
}

// 展示编辑器
void MainWindow::showeditor() { editor->show(); }
