#include <qlogging.h>

#include <TimingTableUsefulWidgets.hpp>
#include <mmm/map/editor/MMapEditor.hpp>
#include <mmm/timing/Timing.hpp>
#include <util/mutil.hpp>
#include <utility>

TimeEditWidget::TimeEditWidget(Timing *&timing, QTableWidget *parent, int index,
                               QIntValidator *validator)
    : QWidget(parent), parent(parent) {
    layout = new QHBoxLayout(this);
    timingIndex = new QLabel(this);
    timeEdit = new QLineEdit(this);
    gotoButton = new QPushButton(this);
    QColor color = Qt::white;
    mutil::set_button_svgcolor(gotoButton, ":/icons/arrow-right.svg", color, 16,
                               16);
    layout->addWidget(timingIndex);
    layout->addWidget(timeEdit);
    layout->addWidget(gotoButton);
    layout->setSpacing(2);
    layout->setContentsMargins(2, 0, 0, 4);
    layout->setStretch(0, 0);
    layout->setStretch(1, 1);
    layout->setStretch(2, 0);
    setLayout(layout);
    timingIndex->setText(QString::number(index) + ":");
    timeEdit->setText(QString::number(timing->timestamp));
    timeEdit->setValidator(validator);
}
TimingParameterEditor::TimingParameterEditor(Timing *&timing,
                                             QTableWidget *parent,
                                             QDoubleValidator *validator)
    : QWidget() {
    layout = new QHBoxLayout(this);
    title = new QLabel(this);
    bpmEdit = new QLineEdit(this);
    bpmEdit->setValidator(validator);
    speedSpinBox = new QDoubleSpinBox(this);
    speedSpinBox->setDecimals(2);
    speedSpinBox->setSuffix("x");
    bpmEdit->setText(QString::number(timing->bpm, 'f', 2));
    layout->addWidget(title);
    if (timing->is_base_timing) {
        speedSpinBox->setValue(1.0);
        title->setText(bpmTitle);
        layout->addWidget(bpmEdit);
        speedSpinBox->hide();
    } else {
        title->setText(speedTitle);
        layout->addWidget(speedSpinBox);
        speedSpinBox->setValue(100.0 / std::abs(timing->beat_length));
        bpmEdit->hide();
    }
    setLayout(layout);
}
TimingSettingWidget::TimingSettingWidget(Timing *&timing, QTableWidget *parent)
    : QWidget(parent) {
    layout = new QHBoxLayout(this);
    deleteButton = new QPushButton(this);
    QColor color = Qt::white;
    mutil::set_button_svgcolor(deleteButton, ":icons/close.svg", color, 16, 16);

    doneButton = new QPushButton(this);
    mutil::set_button_svgcolor(doneButton, ":icons/check.svg", color, 16, 16);
    spacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding,
                             QSizePolicy::Policy::Minimum);
    layout->addItem(spacer);
    layout->addWidget(doneButton);
    layout->addWidget(deleteButton);
    setLayout(layout);
}

TimingRowItem::TimingRowItem(MMap *map, Timing *&timing, QTableWidget *parent,
                             int index, QIntValidator *intvalidator,
                             QDoubleValidator *doublevalidator) {
    this->timing = timing;
    timeEditWgt = new TimeEditWidget(timing, parent, index, intvalidator);
    uninheritedComboBox = new QComboBox(parent);
    uninheritedComboBox->addItems({"true", "false"});
    uninheritedComboBox->setCurrentIndex(timing->is_base_timing ? 0 : 1);
    paramEditor = new TimingParameterEditor(timing, parent, doublevalidator);
    timingSettingWgt = new TimingSettingWidget(timing, parent);

    auto this_cp = this;
    // 完成编辑按钮和删除按钮
    connect(timingSettingWgt->doneButton, &QPushButton::clicked,
            [this_cp, map, parent]() {
                // 检查更新
                auto desTime = this_cp->timeEditWgt->timeEdit->text().toInt();
                auto des_is_base_timing =
                    this_cp->uninheritedComboBox->currentIndex() == 0;
                auto des_param =
                    des_is_base_timing
                        ? this_cp->paramEditor->bpmEdit->text().toDouble()
                        : -100.0 / this_cp->paramEditor->speedSpinBox->value();
                auto param_same =
                    this_cp->timing->is_base_timing
                        ? this_cp->timing->bpm == des_param
                        : 100.0 / std::abs(this_cp->timing->beat_length) ==
                              des_param;
                if (desTime == this_cp->timing->timestamp &&
                    des_is_base_timing == this_cp->timing->is_base_timing &&
                    param_same) {
                } else {
                    // 发送更新指令
                    auto newTiming = this_cp->timing->clone();
                    newTiming->timestamp = desTime;
                    newTiming->is_base_timing = des_is_base_timing;
                    newTiming->bpm =
                        des_is_base_timing ? des_param : this_cp->timing->bpm;
                    newTiming->beat_length = des_is_base_timing
                                                 ? 60000.0 / newTiming->bpm
                                                 : des_param;
                    auto newTimingPtr = newTiming.get();
                    map->editor()->updateTiming(this_cp->timing,
                                                std::move(newTiming));
                    if (desTime != this_cp->timing->timestamp) {
                        // 排序整个表
                        qDebug() << "排序timing表(待实现)";
                        // mutil::sortTableByCustomLogic(parent);
                    }
                    this_cp->timing = newTimingPtr;
                }
            });
    connect(timingSettingWgt->deleteButton, &QPushButton::clicked, []() {
        //
    });
}

AddTimingItem::AddTimingItem(QTableWidget *parent) : parent(parent) {
    setting_widget = new QWidget(parent);
    setting_layout = new QHBoxLayout(parent);
    insertButton = new QPushButton(setting_widget);
    QColor color = Qt::white;
    mutil::set_button_svgcolor(insertButton, ":icons/plus.svg", color, 16, 16);
    setting_spacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding,
                                     QSizePolicy::Policy::Minimum);
    setting_layout->addItem(setting_spacer);
    setting_layout->addWidget(insertButton);
    setting_widget->setLayout(setting_layout);
}
