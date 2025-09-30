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
    initializeGenBeatDivisorsMenu();
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

void MapEditor::initializeGenBeatDivisorsMenu() {
    // 分拍生成按钮
    auto divmenu = new QMenu(ui->generate_divisors_toolbutton);
    auto divmenuwidget = new QWidget();

    // 创建倍率按钮
    auto divratioButton = new QPushButton(divmenu);
    divratioButton->setSizePolicy(QSizePolicy::Expanding,
                                  QSizePolicy::Expanding);
    divratioButton->setText("2x2");
    divratioButton->setToolTip(tr("change div ration"));
    // 分拍乘率滑条
    auto div_multiplier_slider = new QSlider(Qt::Horizontal, divmenuwidget);
    auto div_res_label = new QLabel(tr("4"));
    div_res_label->setAlignment(Qt::AlignmentFlag::AlignCenter);
    div_multiplier_slider->setRange(1, 24);
    div_multiplier_slider->setSingleStep(1);
    div_multiplier_slider->setPageStep(1);
    div_multiplier_slider->setValue(2);

    // 初始化按钮类型尺寸
    divratioButton->setFlat(true);
    divratioButton->setCheckable(true);
    // divratioButton->setMinimumSize(QSize(24, 24));
    // divratioButton->setMaximumSize(QSize(, 24));

    // 布局
    auto *divmenulayout = new QHBoxLayout;
    divmenulayout->setContentsMargins(2, 2, 2, 2);
    divmenulayout->setSpacing(2);
    divmenulayout->addWidget(divratioButton);
    divmenulayout->addWidget(div_multiplier_slider);
    divmenulayout->addWidget(div_res_label);

    divmenuwidget->setLayout(divmenulayout);
    divmenulayout->setStretch(0, 1);
    divmenulayout->setStretch(1, 2);
    divmenulayout->setStretch(2, 1);

    // 默认选中无模式按钮
    divratioButton->setChecked(false);
    divmenu->setContentsMargins(0, 0, 0, 0);

    // 连接信号
    auto this_cp = this;
    connect(divratioButton, &QPushButton::toggled,
            [this_cp, divratioButton, div_multiplier_slider,
             div_res_label](bool checked) {
                int res{0};
                if (checked) {
                    divratioButton->setText(
                        QString("3x%1").arg(div_multiplier_slider->value()));
                    res = 3 * div_multiplier_slider->value();
                } else {
                    divratioButton->setText(
                        QString("2x%1").arg(div_multiplier_slider->value()));
                    res = 2 * div_multiplier_slider->value();
                }
                emit this_cp->updateGeneratedDivisors(res);
                div_res_label->setText(QString::number(res));
            });
    connect(div_multiplier_slider, &QSlider::valueChanged,
            [this_cp, divratioButton, div_multiplier_slider,
             div_res_label](int value) {
                int res{0};
                if (divratioButton->isChecked()) {
                    res = 3 * value;
                    divratioButton->setText(QString("3x%1").arg(value));
                } else {
                    res = 2 * value;
                    divratioButton->setText(QString("2x%1").arg(value));
                }
                emit this_cp->updateGeneratedDivisors(res);
                div_res_label->setText(QString::number(res));
            });

    // 将自定义 Widget 包装成 QWidgetAction
    auto *divwidgetAction = new QWidgetAction(divmenu);
    divwidgetAction->setDefaultWidget(divmenuwidget);

    // 添加到菜单
    divmenu->addAction(divwidgetAction);

    // 设置模式按钮菜单
    ui->generate_divisors_toolbutton->setMenu(divmenu);
}
