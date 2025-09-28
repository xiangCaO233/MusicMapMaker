#include <mapeditor.h>
#include <ui_mapeditor.h>

#include <QLabel>
#include <QMenu>
#include <QSlider>
#include <QWidgetAction>
#include <action/ActionManager.hpp>
#include <canvas/info/MapCanvasInfo.hpp>

void MapEditor::initializeMenus() {
    initializeToolsMenu();
    initializeBgMenu();
}

void MapEditor::bindToolActions() {
    // 获取action
    auto am = ActionManager::instance();
    QAction *handToolAction = am->getAction("canvas.switchhandtool");
    QAction *noteToolAction = am->getAction("canvas.switchnotetool");
    hand_mode_button->setDefaultAction(handToolAction);
    note_mode_button->setDefaultAction(noteToolAction);
}

void MapEditor::updateModeMenuIcon(const QString &actionname) {
    auto am = ActionManager::instance();
    QAction *action = am->getAction(actionname);
    ui->edit_toolsbutton->setIcon(action->icon());
}
//
//
// 工具选择按钮
//
//
void MapEditor::initializeToolsMenu() {
    // 模式选择按钮
    auto modemenu = new QMenu(ui->edit_toolsbutton);
    auto custommodemenuwidget = new QWidget();

    // 创建子模式按钮
    hand_mode_button = new QToolButton();
    note_mode_button = new QToolButton();

    // 初始化按钮类型尺寸
    hand_mode_button->setAutoRaise(true);
    hand_mode_button->setCheckable(true);
    hand_mode_button->setMinimumSize(QSize(24, 24));
    hand_mode_button->setMaximumSize(QSize(24, 24));

    note_mode_button->setAutoRaise(true);
    note_mode_button->setCheckable(true);
    note_mode_button->setMinimumSize(QSize(24, 24));
    note_mode_button->setMaximumSize(QSize(24, 24));

    // 布局
    QVBoxLayout *modemenulayout = new QVBoxLayout;
    modemenulayout->setContentsMargins(0, 0, 0, 0);
    modemenulayout->setSpacing(0);
    modemenulayout->addWidget(hand_mode_button);
    modemenulayout->addWidget(note_mode_button);

    custommodemenuwidget->setLayout(modemenulayout);

    // 默认选中无模式按钮
    hand_mode_button->setChecked(true);

    auto mode_toolbutton = ui->edit_toolsbutton;

    // 监听选中按钮变化
    // connect(modesbuttonGroup,
    //         QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked),
    //         [=](QAbstractButton *button) {
    //             // 切换工具按钮的图标
    //             mode_toolbutton->setIcon(button->icon());
    //             // 切换当前编辑器的模式
    //             if (c->map) {
    //                 auto mode = static_cast<ToolType>(group->id(button));
    //                 switch (mode) {
    //                     case ToolType::HAND: {
    //                         c->use_tool("Hand");
    //                         break;
    //                     }
    //                     case ToolType::NOTE: {
    //                         c->use_tool("Note");
    //                         break;
    //                     }
    //                 }
    //             }
    //         });

    // 将自定义 Widget 包装成 QWidgetAction
    auto *modewidgetAction = new QWidgetAction(modemenu);
    modewidgetAction->setDefaultWidget(custommodemenuwidget);
    modemenu->setContentsMargins(0, 0, 0, 0);

    // 添加到菜单
    modemenu->addAction(modewidgetAction);
    // 设置模式按钮菜单
    ui->edit_toolsbutton->setMenu(modemenu);
}

//
//
// 背景透明度调节按钮
//
//
void MapEditor::initializeBgMenu() {
    // 创建菜单
    auto c = canvas();
    auto mapinfo = c->info<MapCanvasInfo>();

    // 背景控制工具按钮
    auto bgmenu = new QMenu(ui->bg_adjust_toolbutton);
    auto custombgsliderWidget = new QWidget();

    auto bgdarkenwidget = new QWidget();
    auto bg_darken_label = new QLabel(tr("darken"));
    auto bg_darken_slider = new QSlider(Qt::Vertical, bgdarkenwidget);
    bg_darken_slider->setRange(0, 100);
    bg_darken_slider->setValue(20);
    bg_darken_slider->setSizePolicy(QSizePolicy::Expanding,
                                    QSizePolicy::Expanding);
    auto bg_darken_value_label = new QLabel("20");
    // bgopacylabel->setFont(font);
    bg_darken_value_label->setAlignment(Qt::AlignmentFlag::AlignCenter);

    auto bgdarkenlayout = new QVBoxLayout(bgdarkenwidget);
    bgdarkenlayout->setContentsMargins(0, 0, 0, 0);
    bgdarkenlayout->setSpacing(2);
    bgdarkenlayout->addWidget(bg_darken_label);
    bgdarkenlayout->addWidget(bg_darken_slider);
    bgdarkenlayout->addWidget(bg_darken_value_label);
    bgdarkenwidget->setLayout(bgdarkenlayout);

    connect(bg_darken_slider, &QSlider::valueChanged,
            [c, bg_darken_value_label, mapinfo](int value) {
                bg_darken_value_label->setText(QString::number(value));

                mapinfo->mapInfo.darken = float(value) / 100.f;
                c->update_sharedInfo();
            });

    auto bgalphawidget = new QWidget();
    auto bg_alpha_label = new QLabel(tr("alpha"));
    auto bg_alpha_slider = new QSlider(Qt::Vertical, bgalphawidget);
    bg_alpha_slider->setRange(0, 100);
    bg_alpha_slider->setValue(100);
    bg_alpha_slider->setSizePolicy(QSizePolicy::Expanding,
                                   QSizePolicy::Expanding);
    auto bg_alpha_value_label = new QLabel("100");
    // bgopacylabel->setFont(font);
    bg_alpha_value_label->setAlignment(Qt::AlignmentFlag::AlignCenter);

    auto bgalphalayout = new QVBoxLayout(bgalphawidget);
    bgalphalayout->setContentsMargins(0, 0, 0, 0);
    bgalphalayout->setSpacing(2);
    bgalphalayout->addWidget(bg_alpha_label);
    bgalphalayout->addWidget(bg_alpha_slider);
    bgalphalayout->addWidget(bg_alpha_value_label);
    bgalphawidget->setLayout(bgalphalayout);

    connect(bg_alpha_slider, &QSlider::valueChanged,
            [c, bg_alpha_value_label, mapinfo](int value) {
                bg_alpha_value_label->setText(QString::number(value));
                mapinfo->mapInfo.alpha = float(value) / 100.f;
                c->update_sharedInfo();
            });

    auto bgmenulayout = new QHBoxLayout(custombgsliderWidget);
    bgmenulayout->setContentsMargins(2, 2, 2, 2);
    bgmenulayout->setSpacing(2);
    bgmenulayout->addWidget(bgdarkenwidget);
    bgmenulayout->addWidget(bgalphawidget);
    custombgsliderWidget->setLayout(bgmenulayout);

    // 将自定义 Widget 包装成 QWidgetAction
    auto *bgwidgetAction = new QWidgetAction(bgmenu);
    bgwidgetAction->setDefaultWidget(custombgsliderWidget);

    // 添加到菜单
    bgmenu->addAction(bgwidgetAction);
    // 设置背景按钮菜单
    ui->bg_adjust_toolbutton->setMenu(bgmenu);
}
