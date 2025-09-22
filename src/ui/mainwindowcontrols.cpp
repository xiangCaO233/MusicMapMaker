#include <audio/track/trackmanager.h>
#include <mainwindow.h>
#include <project/projectmanager.h>
#include <qobjectdefs.h>
#include <ui_mainwindow.h>

#include <action/ActionManager.hpp>
#include <action/modules/canvas/EditorActionHandler.hpp>
#include <action/modules/canvas/EditorActions.hpp>
#include <action/modules/file/FileActionHandler.hpp>
#include <action/modules/file/FileActions.hpp>

// 初始化所有的action
void MainWindow::initActions() {
    auto am = ActionManager::instance();
    // 文件菜单
    FileActions::createActions();
    auto fileHandler = FileActionHandler::instance();
    am->connectCommand("project.new", fileHandler, SLOT(onNewProject()));
    am->connectCommand("file.new", fileHandler, SLOT(onNewFile()));
    am->connectCommand("file.open", fileHandler, SLOT(onOpen()));
    am->connectCommand("file.openDirectory", fileHandler,
                       SLOT(onOpenDirectory()));

    am->connectCommand("file.save", fileHandler, SLOT(onSave()));
    am->connectCommand("file.saveas", fileHandler, SLOT(onSaveAs()));
    am->connectCommand("file.export", fileHandler, SLOT(onExport()));

    ui->menuFile_F->addAction(am->getAction("project.new"));
    ui->menuFile_F->addAction(am->getAction("file.new"));
    ui->menuFile_F->addAction(am->getAction("file.open"));
    ui->menuFile_F->addAction(am->getAction("file.openDirectory"));
    ui->menuFile_F->addSeparator();
    ui->menuFile_F->addAction(am->getAction("file.save"));
    ui->menuFile_F->addAction(am->getAction("file.saveas"));
    ui->menuFile_F->addAction(am->getAction("file.export"));

    // 编辑菜单
    EditorActions::createActions();
    auto editHandler = EditorActionHandler::instance();
    am->connectCommand("canvas.pause_or_resume", editHandler,
                       SLOT(onPause_Resume()));
    am->connectCommand("canvas.cancel", editHandler, SLOT(onCancel()));
    am->connectCommand("canvas.selectpage", editHandler, SLOT(onSelectPage()));
    am->connectCommand("canvas.selectall", editHandler, SLOT(onSelectAll()));
    am->connectCommand("canvas.cut", editHandler, SLOT(onCut()));
    am->connectCommand("canvas.copy", editHandler, SLOT(onCopy()));
    am->connectCommand("canvas.paste", editHandler, SLOT(onPaste()));
    am->connectCommand("canvas.delete", editHandler, SLOT(onDelete()));
    am->connectCommand("canvas.undo", editHandler, SLOT(onUndo()));
    am->connectCommand("canvas.redo", editHandler, SLOT(onRedo()));
    am->connectCommand("canvas.find", editHandler, SLOT(onFind()));

    ui->menuEdit_E->addAction(am->getAction("canvas.pause_or_resume"));
    ui->menuEdit_E->addAction(am->getAction("canvas.cancel"));
    ui->menuEdit_E->addSeparator();
    ui->menuEdit_E->addAction(am->getAction("canvas.selectpage"));
    ui->menuEdit_E->addAction(am->getAction("canvas.selectall"));
    ui->menuEdit_E->addSeparator();
    ui->menuEdit_E->addAction(am->getAction("canvas.cut"));
    ui->menuEdit_E->addAction(am->getAction("canvas.copy"));
    ui->menuEdit_E->addAction(am->getAction("canvas.paste"));
    ui->menuEdit_E->addAction(am->getAction("canvas.delete"));
    ui->menuEdit_E->addSeparator();
    ui->menuEdit_E->addAction(am->getAction("canvas.undo"));
    ui->menuEdit_E->addAction(am->getAction("canvas.redo"));
    ui->menuEdit_E->addSeparator();
    ui->menuEdit_E->addAction(am->getAction("canvas.find"));
}

// 菜单项槽函数 - 控制显示和隐藏
// void MainWindow::on_actionTrack_Manager_toggled(bool checked) {
//     ui->track_manager_dock->setVisible(checked);
// }
//
// void MainWindow::on_actionProject_Manager_toggled(bool checked) {
//     ui->project_dock->setVisible(checked);
// }
