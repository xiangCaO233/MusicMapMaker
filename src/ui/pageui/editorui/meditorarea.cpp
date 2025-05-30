#include "meditorarea.h"

#include <qapplication.h>
#include <qbuttongroup.h>
#include <qfiledialog.h>
#include <qlabel.h>
#include <qmenu.h>
#include <qnamespace.h>
#include <qslider.h>
#include <qtmetamacros.h>
#include <qwidgetaction.h>

#include <filesystem>
#include <string>

#include "../../../canvas/map/editor/MapEditor.h"
#include "../../../mmm/MapWorkProject.h"
#include "../../../mmm/map/rm/RMMap.h"
#include "../../../util/mutil.h"
#include "colorful-log.h"
#include "mmm/map/osu/OsuMap.h"
#include "ui_meditorarea.h"

MEditorArea::MEditorArea(QWidget *parent)
    : QWidget(parent), ui(new Ui::MEditorArea) {
    ui->setupUi(this);
    canvas_container = ui->canvas_container;
    // 初始化工具按钮菜单
    initialize_toolbuttons();
    // 默认隐藏音频控制器
    // ui->audio_time_controller->hide();
    initialize_signals();
}

MEditorArea::~MEditorArea() { delete ui; }

// 初始化所有信号连接
void MEditorArea::initialize_signals() {
    // 连接快捷键切换模式槽
    connect(canvas_container->canvas.data(),
            &MapWorkspaceCanvas::switch_edit_mode, this,
            &MEditorArea::on_canvasSwitchmode);

    // 连接画布时间更新信号
    connect(ui->canvas_container->canvas.data(),
            &MapWorkspaceCanvas::current_time_stamp_changed, this,
            &MEditorArea::on_canvasTimestampChanged);

    // 连接时间控制器暂停信号到效果线程
    // connect(ui->audio_time_controller, &TimeController::pause_button_changed,
    //         canvas_container->canvas->effect_thread,
    //         &EffectThread::on_canvas_pause);
}

// 使用主题
void MEditorArea::use_theme(GlobalTheme theme) {
    current_theme = theme;
    QColor button_color;
    switch (theme) {
        case GlobalTheme::DARK: {
            button_color = QColor(255, 255, 255);
            break;
        }
        case GlobalTheme::LIGHT: {
            button_color = QColor(0, 0, 0);
            break;
        }
    }

    // 设置工具栏按钮图标颜色
    mutil::set_toolbutton_svgcolor(ui->default_divisor_policy_toolbutton,
                                   ":/icons/stream.svg", button_color, 12, 12);
    mutil::set_toolbutton_svgcolor(
        ui->bookmark_toolbutton, ":/icons/bookmark.svg", button_color, 12, 12);
    mutil::set_toolbutton_svgcolor(ui->mode_toolbutton, ":/icons/eye.svg",
                                   button_color, 12, 12);
    mutil::set_toolbutton_svgcolor(ui->background_opacy_toolbutton,
                                   ":/icons/background.svg", button_color, 12,
                                   12);
    mutil::set_toolbutton_svgcolor(ui->adjust_judgeline_position_toolbutton,
                                   ":/icons/judgeline.svg", button_color, 12,
                                   12);
    mutil::set_toolbutton_svgcolor(ui->adjust_visual_offset_toolbutton,
                                   ":/icons/visual-offset.svg", button_color,
                                   12, 12);

    mutil::set_button_svgcolor(ui->magnet_todivisor_button,
                               ":/icons/magnet.svg", button_color, 16, 16);

    mutil::set_button_svgcolor(ui->show_object_after_judgeline_button,
                               ":/icons/glasses.svg", button_color, 16, 16);

    mutil::set_button_svgcolor(ui->show_timeline_button, ":/icons/lines.svg",
                               button_color, 16, 16);

    // 模式选择菜单内的按钮组
    mutil::set_button_svgcolor(drawnote_mode_button, ":/icons/drawnote.svg",
                               button_color, 16, 16);
    mutil::set_button_svgcolor(drawline_mode_button, ":/icons/drawline.svg",
                               button_color, 16, 16);
    mutil::set_button_svgcolor(place_timing_mode_button, ":/icons/timing.svg",
                               button_color, 16, 16);
    mutil::set_button_svgcolor(selection_mode_button, ":/icons/selection.svg",
                               button_color, 16, 16);
    mutil::set_button_svgcolor(none_mode_button, ":/icons/eye.svg",
                               button_color, 16, 16);

    // 两个状态
    mutil::set_button_svgcolor(ui->pausebutton, ":/icons/play.svg",
                               button_color, 16, 16);
    mutil::set_button_svgcolor(ui->wheel_direction_button,
                               ":/icons/long-arrow-alt-up.svg", button_color,
                               16, 16);
    mutil::set_button_svgcolor(ui->lock_edit_mode_button,
                               ":/icons/lock-open.svg", button_color, 16, 16);

    ratio_button->setStyleSheet("color: " + button_color.name());

    // 设置画布区主题
    ui->canvas_container->use_theme(theme);
}

// 初始化工具按钮菜单
void MEditorArea::initialize_toolbuttons() {
    auto canvas = canvas_container->canvas.data();
    QFont font;
    font.setPointSize(8);
    //
    //
    // 模式选择按钮
    auto modemenu = new QMenu(ui->mode_toolbutton);
    auto custommodemenuwidget = new QWidget();

    // 创建按钮组
    modesbuttonGroup = new QButtonGroup(this);
    // 设置独占模式（单选）
    modesbuttonGroup->setExclusive(true);

    // 创建子模式按钮
    drawnote_mode_button = new QPushButton;
    drawline_mode_button = new QPushButton;
    place_timing_mode_button = new QPushButton;
    selection_mode_button = new QPushButton;
    none_mode_button = new QPushButton;

    // 初始化按钮类型尺寸
    drawnote_mode_button->setFlat(true);
    drawnote_mode_button->setCheckable(true);
    drawnote_mode_button->setMinimumSize(QSize(24, 24));
    drawnote_mode_button->setMaximumSize(QSize(24, 24));
    drawnote_mode_button->setToolTip(tr("Place note mode"));

    drawline_mode_button->setFlat(true);
    drawline_mode_button->setCheckable(true);
    drawline_mode_button->setMinimumSize(QSize(24, 24));
    drawline_mode_button->setMaximumSize(QSize(24, 24));
    drawline_mode_button->setToolTip(tr("Place line and slide mode"));

    place_timing_mode_button->setFlat(true);
    place_timing_mode_button->setCheckable(true);
    place_timing_mode_button->setMinimumSize(QSize(24, 24));
    place_timing_mode_button->setMaximumSize(QSize(24, 24));
    place_timing_mode_button->setToolTip(tr("Place timing mode"));

    selection_mode_button->setFlat(true);
    selection_mode_button->setCheckable(true);
    selection_mode_button->setMinimumSize(QSize(24, 24));
    selection_mode_button->setMaximumSize(QSize(24, 24));
    selection_mode_button->setToolTip(tr("Selection mode"));

    none_mode_button->setFlat(true);
    none_mode_button->setCheckable(true);
    none_mode_button->setMinimumSize(QSize(24, 24));
    none_mode_button->setMaximumSize(QSize(24, 24));
    none_mode_button->setToolTip(tr("Observer mode"));

    // 将按钮添加到按钮组
    // 第二个参数是按钮ID
    modesbuttonGroup->addButton(
        drawnote_mode_button, static_cast<int32_t>(MouseEditMode::PLACE_NOTE));
    modesbuttonGroup->addButton(
        drawline_mode_button,
        static_cast<int32_t>(MouseEditMode::PLACE_LONGNOTE));
    modesbuttonGroup->addButton(
        place_timing_mode_button,
        static_cast<int32_t>(MouseEditMode::PLACE_TIMING));
    modesbuttonGroup->addButton(selection_mode_button,
                                static_cast<int32_t>(MouseEditMode::SELECT));
    modesbuttonGroup->addButton(none_mode_button,
                                static_cast<int32_t>(MouseEditMode::NONE));

    // 布局
    QVBoxLayout *modemenulayout = new QVBoxLayout;
    modemenulayout->setContentsMargins(0, 0, 0, 0);
    modemenulayout->setSpacing(0);
    modemenulayout->addWidget(drawnote_mode_button);
    modemenulayout->addWidget(drawline_mode_button);
    modemenulayout->addWidget(place_timing_mode_button);
    modemenulayout->addWidget(selection_mode_button);
    modemenulayout->addWidget(none_mode_button);

    custommodemenuwidget->setLayout(modemenulayout);

    // 默认选中无模式按钮
    none_mode_button->setChecked(true);

    auto c = canvas_container->canvas.data();
    auto mode_toolbutton = ui->mode_toolbutton;
    auto group = modesbuttonGroup;

    // 监听选中按钮变化
    connect(modesbuttonGroup,
            QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked),
            [=](QAbstractButton *button) {
                // 切换工具按钮的图标
                mode_toolbutton->setIcon(button->icon());
                // 切换当前编辑器的模式
                if (c->working_map) {
                    c->editor->edit_mode =
                        static_cast<MouseEditMode>(group->id(button));
                }
            });

    // 将自定义 Widget 包装成 QWidgetAction
    auto *modewidgetAction = new QWidgetAction(modemenu);
    modewidgetAction->setDefaultWidget(custommodemenuwidget);
    modemenu->setContentsMargins(0, 0, 0, 0);

    // 添加到菜单
    modemenu->addAction(modewidgetAction);
    // 设置模式按钮菜单
    ui->mode_toolbutton->setMenu(modemenu);

    //
    //
    // 分拍策略按钮
    // 菜单
    auto divisormenu = new QMenu(ui->default_divisor_policy_toolbutton);
    auto customdivosorsliderWidget = new QWidget();

    // 调整倍率按钮
    static int div_ratio = 2;
    static int slide_ratio = 1;

    ratio_button = new QPushButton("2", customdivosorsliderWidget);
    ratio_button->setMinimumSize(QSize(24, 24));
    ratio_button->setMaximumSize(QSize(24, 24));
    ratio_button->setToolTip(tr("Change beat type"));

    QPushButton *cp_ratio_button = ratio_button;

    // 显示的分拍策略
    div_res_label = new QLabel(customdivosorsliderWidget);
    div_res_label->setText("1/2");
    div_res_label->setFont(font);
    div_res_label->setAlignment(Qt::AlignmentFlag::AlignCenter);
    QLabel *cp_div_res_label = div_res_label;

    connect(ratio_button, &QPushButton::clicked, [=]() {
        if (div_ratio == 2) {
            div_ratio = 3;
            cp_ratio_button->setText("3");
        } else {
            div_ratio = 2;
            cp_ratio_button->setText("2");
        }
        auto res_div = div_ratio * slide_ratio;
        cp_div_res_label->setText(QString("1/%1").arg(res_div));
        if (canvas->working_map) {
            canvas->working_map->project_reference->config.default_divisors =
                res_div;
            canvas->working_map->project_reference->canvasconfig_node
                .attribute("default-divisors")
                .set_value(res_div);
        }
    });

    // 分拍调整滑条
    divisorslider = new QSlider(Qt::Vertical, customdivosorsliderWidget);
    divisorslider->setMinimum(1);
    divisorslider->setMaximum(16);
    divisorslider->setPageStep(1);
    divisorslider->setSingleStep(1);

    connect(divisorslider, &QSlider::valueChanged, [=](int value) {
        slide_ratio = value;
        auto res_div = div_ratio * slide_ratio;
        cp_div_res_label->setText(QString("1/%1").arg(res_div));
        if (canvas->working_map) {
            canvas->working_map->project_reference->config.default_divisors =
                res_div;
            canvas->working_map->project_reference->canvasconfig_node
                .attribute("default-divisors")
                .set_value(res_div);
        }
    });
    // 布局
    auto divmenulayout = new QVBoxLayout(customdivosorsliderWidget);
    divmenulayout->setContentsMargins(2, 2, 2, 2);
    divmenulayout->setSpacing(2);
    divmenulayout->addWidget(ratio_button);
    divmenulayout->addWidget(div_res_label);
    divmenulayout->addWidget(divisorslider);
    customdivosorsliderWidget->setLayout(divmenulayout);
    // 将自定义 Widget 包装成 QWidgetAction
    auto *divwidgetAction = new QWidgetAction(divisormenu);
    divwidgetAction->setDefaultWidget(customdivosorsliderWidget);

    // 添加到菜单
    divisormenu->addAction(divwidgetAction);
    // 设置分拍策略按钮菜单
    ui->default_divisor_policy_toolbutton->setMenu(divisormenu);

    //
    //
    // 背景透明度调节按钮
    //
    //
    // 创建菜单
    auto bgmenu = new QMenu(ui->background_opacy_toolbutton);
    auto custombgsliderWidget = new QWidget();
    auto bgslider = new QSlider(Qt::Vertical, custombgsliderWidget);
    bgslider->setRange(0, 100);
    bgslider->setValue(25);
    auto bgopacylabel = new QLabel("25");
    bgopacylabel->setFont(font);
    bgopacylabel->setAlignment(Qt::AlignmentFlag::AlignCenter);

    auto bgmenulayout = new QVBoxLayout(custombgsliderWidget);
    bgmenulayout->setContentsMargins(2, 2, 2, 2);
    bgmenulayout->setSpacing(2);
    bgmenulayout->addWidget(bgslider);
    bgmenulayout->addWidget(bgopacylabel);
    custombgsliderWidget->setLayout(bgmenulayout);

    connect(bgslider, &QSlider::valueChanged, [=](int value) {
        bgopacylabel->setText(QString::number(value));
        canvas->editor->cstatus.background_darken_ratio =
            1.0 - double(value) / 100.0;
    });

    // 将自定义 Widget 包装成 QWidgetAction
    auto *bgwidgetAction = new QWidgetAction(bgmenu);
    bgwidgetAction->setDefaultWidget(custombgsliderWidget);

    // 添加到菜单
    bgmenu->addAction(bgwidgetAction);
    // 设置背景按钮菜单
    ui->background_opacy_toolbutton->setMenu(bgmenu);

    //
    //
    // 轨道高度调节按钮
    //
    //
    // 创建菜单
    auto jposmenu = new QMenu(ui->adjust_judgeline_position_toolbutton);

    auto customjgsliderWidget = new QWidget(jposmenu);
    auto jgslider = new QSlider(Qt::Vertical, customjgsliderWidget);
    jgslider->setRange(0, 100);
    jgslider->setValue(16);
    auto jgposlabel = new QLabel("16");
    jgposlabel->setFont(font);
    jgposlabel->setAlignment(Qt::AlignmentFlag::AlignCenter);

    auto jgmenulayout = new QVBoxLayout(customjgsliderWidget);
    jgmenulayout->setContentsMargins(2, 2, 2, 2);
    jgmenulayout->setSpacing(2);
    jgmenulayout->addWidget(jgslider);
    jgmenulayout->addWidget(jgposlabel);
    customjgsliderWidget->setLayout(jgmenulayout);

    connect(jgslider, &QSlider::valueChanged, [=](int value) {
        jgposlabel->setText(QString::number(value));
        canvas->editor->csettings.judgeline_position = double(value) / 100.0;
        canvas->editor->update_size(canvas->size());
    });

    // 将自定义 Widget 包装成 QWidgetAction
    auto *jgwidgetAction = new QWidgetAction(jposmenu);
    jgwidgetAction->setDefaultWidget(customjgsliderWidget);

    // 添加到菜单
    jposmenu->addAction(jgwidgetAction);
    // 设置背景按钮菜单
    ui->adjust_judgeline_position_toolbutton->setMenu(jposmenu);

    //
    //
    // 视觉偏移调节按钮
    //
    //
    // 创建菜单
    auto vposmenu = new QMenu(ui->adjust_visual_offset_toolbutton);

    auto customvpossliderWidget = new QWidget(vposmenu);
    auto vposslider = new QSlider(Qt::Vertical, customjgsliderWidget);
    vposslider->setRange(-200, 200);
    vposslider->setValue(0);
    vposslider->setSingleStep(1);
    vposslider->setPageStep(5);
    auto vposlabel = new QLabel("0");
    vposlabel->setFont(font);
    vposlabel->setAlignment(Qt::AlignmentFlag::AlignCenter);

    auto vposmenulayout = new QVBoxLayout(customvpossliderWidget);
    vposmenulayout->setContentsMargins(0, 0, 0, 0);
    vposmenulayout->setSpacing(2);
    vposmenulayout->addWidget(vposslider);
    vposmenulayout->addWidget(vposlabel);

    customvpossliderWidget->setLayout(vposmenulayout);

    connect(vposslider, &QSlider::valueChanged, [=](int value) {
        vposlabel->setText(QString::number(value));
        canvas->editor->cstatus.graphic_offset = value;
        canvas->editor->update_size(canvas->size());
    });

    // 将自定义 Widget 包装成 QWidgetAction
    auto *vposwidgetAction = new QWidgetAction(vposmenu);
    vposwidgetAction->setDefaultWidget(customvpossliderWidget);

    // 添加到菜单
    vposmenu->addAction(vposwidgetAction);
    // 设置背景按钮菜单
    ui->adjust_visual_offset_toolbutton->setMenu(vposmenu);

    //
    //
    // 书签按钮
    update_bookmarks();
}

// 更新书签
void MEditorArea::update_bookmarks() {}

// 画布时间变化事件
void MEditorArea::oncanvas_timestampChanged(double time) {
    if (!canvas_container->canvas->working_map) return;
    // 计算进度
    auto progress = time / canvas_container->canvas->working_map->map_length;

    // 设置进度条
    ui->playprogress->setValue(ui->playprogress->maximum() * progress);
}

// 画布暂停槽
void MEditorArea::on_canvasPause([[maybe_unused]] bool paused) {
    update_pause_button();
}

// 更新暂停按钮
void MEditorArea::update_pause_button() {
    QColor button_color;
    switch (current_theme) {
        case GlobalTheme::DARK: {
            button_color = QColor(255, 255, 255);
            break;
        }
        case GlobalTheme::LIGHT: {
            button_color = QColor(0, 0, 0);
            break;
        }
    }
    mutil::set_button_svgcolor(
        ui->pausebutton,
        (canvas_container->canvas->editor->cstatus.canvas_pasued
             ? ":/icons/play.svg"
             : ":/icons/pause.svg"),
        button_color, 16, 16);
}

// 暂停按钮
void MEditorArea::on_pausebutton_clicked() {
    if (canvas_container->canvas->working_map &&
        canvas_container->canvas->working_map->project_reference->devicename !=
            "unknown output device") {
        canvas_container->canvas->editor->cstatus.canvas_pasued =
            !canvas_container->canvas->editor->cstatus.canvas_pasued;
        update_pause_button();

        emit pause_button_changed(
            canvas_container->canvas->editor->cstatus.canvas_pasued);
    }
}

// 画布通过快捷键切换模式
void MEditorArea::on_canvasSwitchmode(MouseEditMode mode) {
    // 选中对应按钮
    auto button = modesbuttonGroup->button(static_cast<int32_t>(mode));
    button->setChecked(true);
    // 切换工具按钮的图标
    ui->mode_toolbutton->setIcon(button->icon());
}

// 画布时间变化事件
// 更新进度条
void MEditorArea::on_canvasTimestampChanged(double time) {
    double t = 0.0;
    double v = 0.0;
    if (ui->canvas_container->canvas.data()->working_map) {
        // 更新进度条
        auto maptime = (double)(ui->canvas_container->canvas.data()
                                    ->working_map->map_length);
        double ratio = time / maptime;
        t = ratio * 10000.0;
        v = ui->playprogress->maximum() * ratio;
    }
    sync_time_lock = true;
    ui->progress_slider->setValue(int(t));
    ui->playprogress->setValue(int(v));
}

// page选择了新map事件
void MEditorArea::on_selectnewmap(std::shared_ptr<MMap> &map) {
    // 为画布切换map
    ui->canvas_container->canvas.data()->switch_map(map);

    // 发送个信号
    emit switched_map(map);

    // 更新一下
    ui->canvas_container->canvas.data()->update();

    // 根据项目配置更新工作区选项和配置
}

// 滚动方向切换按钮触发
void MEditorArea::on_wheel_direction_button_toggled(bool checked) {
    ui->canvas_container->canvas.data()->editor->cstatus.scroll_direction =
        (checked ? -1.0 : 1.0);

    // 根据主题切换图标颜色
    QColor file_button_color;
    switch (current_theme) {
        case GlobalTheme::DARK: {
            file_button_color = QColor(255, 255, 255);
            break;
        }
        case GlobalTheme::LIGHT: {
            file_button_color = QColor(0, 0, 0);
            break;
        }
    }

    // 两个状态
    mutil::set_button_svgcolor(ui->wheel_direction_button,
                               (checked ? ":/icons/long-arrow-alt-down.svg"
                                        : ":/icons/long-arrow-alt-up.svg"),
                               file_button_color, 16, 16);
}

// 吸附到分拍线按钮状态切换事件
void MEditorArea::on_magnet_todivisor_button_toggled(bool checked) {
    ui->canvas_container->canvas.data()->editor->cstatus.is_magnet_to_divisor =
        checked;
}

// 模式锁定按钮状态切换事件
void MEditorArea::on_lock_edit_mode_button_toggled(bool checked) {
    lock_mode_auto_switch = checked;
    // 根据主题切换图标颜色
    QColor file_button_color;
    switch (current_theme) {
        case GlobalTheme::DARK: {
            file_button_color = QColor(255, 255, 255);
            break;
        }
        case GlobalTheme::LIGHT: {
            file_button_color = QColor(0, 0, 0);
            break;
        }
    }

    // 两个状态
    mutil::set_button_svgcolor(
        ui->lock_edit_mode_button,
        (checked ? ":/icons/lock.svg" : ":/icons/lock-open.svg"),
        file_button_color, 16, 16);
}

// 进度条移动事件
void MEditorArea::on_progress_slider_valueChanged(int value) {
    if (sync_time_lock) {
        sync_time_lock = false;
        return;
    }
    // 同步画布时间
    if (ui->canvas_container->canvas.data()->is_paused()) {
        // 不暂停不允许调节时间
        if (ui->canvas_container->canvas.data()->working_map) {
            double ratio = (double)value / 10000.0;
            auto maptime = (double)(ui->canvas_container->canvas.data()
                                        ->working_map->map_length);
            ui->canvas_container->canvas.data()
                ->editor->cstatus.current_time_stamp = maptime * ratio;
            ui->canvas_container->canvas.data()
                ->editor->cstatus.current_visual_time_stamp =
                ui->canvas_container->canvas.data()
                    ->editor->cstatus.current_time_stamp +
                ui->canvas_container->canvas.data()
                    ->editor->cstatus.graphic_offset;
            ui->canvas_container->canvas.data()->played_effects_objects.clear();
            emit progress_pos_changed(maptime * ratio);
        }
    }
}

// 切换音频控制器
// void MEditorArea::on_audio_time_controller_button_clicked() {
//   ui->audio_time_controller->setHidden(!ui->audio_time_controller->isHidden());
// }

void MEditorArea::on_show_object_after_judgeline_button_toggled(bool checked) {
    canvas_container->canvas.get()
        ->editor->csettings.show_object_after_judgeline = checked;
}

void MEditorArea::on_show_timeline_button_toggled(bool checked) {
    canvas_container->canvas.get()->editor->csettings.show_timeline = checked;
}

// 保存action
void MEditorArea::on_saveMap() {}

void MEditorArea::on_saveMapAs() {
    auto &map = canvas_container->canvas.data()->working_map;
    if (map) {
        QMap<QString, QString> formats;
        auto mmmf = tr("mmm mapfile");
        auto imdf = tr("imd mapfile");
        auto osuf = tr("osu mapfile");
        formats[mmmf] = ".mmm";
        formats[imdf] = ".imd";
        formats[osuf] = ".osu";

        QMap<QString, QString> defaultNames;

        defaultNames[mmmf] = QString::fromStdString(mutil::sanitizeFilename(
            map->title_unicode + "-" + std::to_string(map->orbits) + "k-" +
            map->version));
        defaultNames[imdf] = QString::fromStdString(mutil::sanitizeFilename(
            map->title_unicode + "_" + std::to_string(map->orbits) + "k_" +
            map->version));
        defaultNames[osuf] = QString::fromStdString(mutil::sanitizeFilename(
            map->artist + " - " + map->title + "(" + map->author + ")" + "[" +
            map->version + "]"));

        // 指定PNG Image作为默认格式
        auto selected_file = mutil::getSaveDirectoryWithFilename(
            this, tr("Save As"), tr("File Formats:"), formats, defaultNames,
            mmmf);

        if (selected_file != "") {
            XINFO("尝试保存到:" + selected_file.toStdString());
            map->write_to_file(selected_file.toStdString().c_str());
        }
    }
}
