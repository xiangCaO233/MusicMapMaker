#include <audio/track/trackmanager.h>
#include <colorful-log.h>
#include <mainwindow.h>
#include <project/projectmanager.h>
#include <qobjectdefs.h>
#include <ui_mainwindow.h>

#include <QActionGroup>
#include <action/ActionManager.hpp>
#include <action/modules/canvas/EditorActionHandler.hpp>
#include <action/modules/canvas/EditorActions.hpp>
#include <action/modules/file/FileActionHandler.hpp>
#include <action/modules/file/FileActions.hpp>
#include <map/MapCanvas.hpp>

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
    am->connectCommand("canvas.switchhandtool", editHandler,
                       SLOT(onSwitch_Handtool()));
    am->connectCommand("canvas.switchnotetool", editHandler,
                       SLOT(onSwitch_Notetool()));
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

    // 创建并填充 "切换工具" 子菜单
    // 给子菜单指定一个父对象(ui->menuEdit_E)，Qt会负责它的内存管理
    QMenu *toolsMenu = new QMenu(tr("Switch Tool"), ui->menuEdit_E);
    auto handToolAction = am->getAction("canvas.switchhandtool");
    auto noteToolAction = am->getAction("canvas.switchnotetool");
    // 切换工具action
    MapCanvas *canvas = ui->editor->canvas();
    auto editui = ui->editor;
    connect(editHandler, &EditorActionHandler::switch_handtool,
            [canvas, editui]() {
                canvas->use_tool("Hand");
                editui->updateModeMenuIcon("canvas.switchhandtool");
                XINFO("切换hand工具");
            });
    connect(editHandler, &EditorActionHandler::switch_notetool,
            [canvas, editui]() {
                canvas->use_tool("Note");
                editui->updateModeMenuIcon("canvas.switchnotetool");
                XINFO("切换note工具");
            });

    // 使用 Action Group 来管理工具的互斥状态
    QActionGroup *toolActionGroup = new QActionGroup(toolsMenu);
    toolActionGroup->setExclusive(true);
    handToolAction->setCheckable(true);
    noteToolAction->setCheckable(true);
    toolActionGroup->addAction(handToolAction);
    toolActionGroup->addAction(noteToolAction);
    handToolAction->setChecked(true);  // 设置一个默认选中的工具

    toolsMenu->addAction(handToolAction);
    toolsMenu->addAction(noteToolAction);

    // 将子菜单添加到主菜单中
    ui->menuEdit_E->addMenu(toolsMenu);

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

    // 将主题选项action包裹一下
    QActionGroup *themeActionGroup = new QActionGroup(ui->menuTheme_T);
    themeActionGroup->setExclusive(true);
    themeActionGroup->addAction(ui->actionOpen_Source_Dark);
    themeActionGroup->addAction(ui->actionOpen_Source_Light);
    themeActionGroup->addAction(ui->actionColin_Dark);
    themeActionGroup->addAction(ui->actionColin_Light);
    ui->actionOpen_Source_Dark->setCheckable(true);
    ui->actionOpen_Source_Light->setCheckable(true);
    ui->actionColin_Dark->setCheckable(true);
    ui->actionColin_Light->setCheckable(true);

    auto this_cp = this;

    // 处理用户点击，防止取消选中
    connect(ui->actionOpen_Source_Dark, &QAction::triggered, [this_cp]() {
        if (this_cp->ui->actionOpen_Source_Dark->isChecked())
            return;  // 如果已经选中，就什么都不做
        this_cp->ui->actionOpen_Source_Dark->setChecked(
            true);  // 否则，设为选中
    });
    connect(ui->actionOpen_Source_Light, &QAction::triggered, [this_cp]() {
        if (this_cp->ui->actionOpen_Source_Light->isChecked()) return;
        this_cp->ui->actionOpen_Source_Light->setChecked(true);
    });
    connect(ui->actionColin_Dark, &QAction::triggered, [this_cp]() {
        if (this_cp->ui->actionColin_Dark->isChecked()) return;
        this_cp->ui->actionColin_Dark->setChecked(true);
    });
    connect(ui->actionColin_Light, &QAction::triggered, [this_cp]() {
        if (this_cp->ui->actionColin_Light->isChecked()) return;
        this_cp->ui->actionColin_Light->setChecked(true);
    });

    connect(ui->actionOpen_Source_Dark, &QAction::toggled,
            [this_cp](bool checked) {
                if (checked) {
                    this_cp->use_theme(GlobalTheme::OPEN_DARK);
                }
            });
    connect(ui->actionOpen_Source_Light, &QAction::toggled,
            [this_cp](bool checked) {
                if (checked) {
                    this_cp->use_theme(GlobalTheme::OPEN_LIGHT);
                }
            });
    connect(ui->actionColin_Dark, &QAction::toggled, [this_cp](bool checked) {
        if (checked) {
            this_cp->use_theme(GlobalTheme::COLIN_DARK);
        }
    });
    connect(ui->actionColin_Light, &QAction::toggled, [this_cp](bool checked) {
        if (checked) {
            this_cp->use_theme(GlobalTheme::COLIN_LIGHT);
        }
    });
}
