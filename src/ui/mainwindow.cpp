#include "mainwindow.h"

#include <audio/track/trackmanager.h>
#include <project/projectmanager.h>
#include <ui_mainwindow.h>

#include <canvas/map/MapCanvas.hpp>
#include <mmm/obj/Note.hpp>
#include <mmm/project/MProject.hpp>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    editor = new MapEditor();
    projectmanager = new ProjectManager();
    trackmanager = new TrackManager();

    editor->hide();
    projectmanager->hide();
    trackmanager->hide();

    // 捕获ui指针
    auto capui = ui;

    connect(editor, &MapEditor::close_signal,
            [capui]() { capui->actionEditor->setChecked(false); });
    connect(trackmanager, &TrackManager::close_signal,
            [capui]() { capui->actionTrack_Manager->setChecked(false); });
    connect(projectmanager, &ProjectManager::close_signal,
            [capui]() { capui->actionProject_Manager->setChecked(false); });

    // auto project = std::make_shared<MProject>(
    //     static_cast<TextureLoadCallback*>(editor->canvas()));

    // std::thread t([=]() {
    //     std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    //     project->open("/Users/2333xiang/Downloads/Juggernaut. - Antler");
    // });
    // t.detach();
    MMap map(
        "/Users/2333xiang/Downloads/Juggernaut. - Antler/Juggernaut. - Antler "
        "(xiang_233) [NOInsane].osu");
    auto notes = map.note_set().get_all_notes_ordered();
    for (const auto& handle : notes) {
        qDebug() << map.note_set().get_note(handle)->toString();
    }
}

MainWindow::~MainWindow() {
    // editor->hide();
    // projectmanager->hide();
    // trackmanager->hide();

    delete editor;
    delete trackmanager;
    delete projectmanager;
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent* e) {
    editor->close();
    trackmanager->close();
    projectmanager->close();
}
