#include <mapeditor.h>
#include <ui_mapeditor.h>

#include <QLabel>
#include <QMenu>
#include <QSlider>
#include <QWidgetAction>
#include <canvas/info/MapCanvasInfo.hpp>

void MapEditor::initializeMenus() const {
    initializeToolsMenu();
    initializeBgMenu();
}

void MapEditor::initializeToolsMenu() const {}

//
//
// 背景透明度调节按钮
//
//
void MapEditor::initializeBgMenu() const {
    // 创建菜单
    auto c = canvas();
    auto mapinfo = c->info<MapCanvasInfo>();
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
    bg_alpha_slider->setValue(80);
    bg_alpha_slider->setSizePolicy(QSizePolicy::Expanding,
                                   QSizePolicy::Expanding);
    auto bg_alpha_value_label = new QLabel("80");
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
