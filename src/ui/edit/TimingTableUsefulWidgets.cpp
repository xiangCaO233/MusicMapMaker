#include <qlogging.h>

#include <TimingTableUsefulWidgets.hpp>
#include <mmm/map/editor/MMapEditor.hpp>
#include <mmm/timing/Timing.hpp>
#include <util/mutil.hpp>

TimeEditWidget::TimeEditWidget(Timing *&timing, QTableWidget *parent,
                               QIntValidator *validator)
    : QWidget(parent), parent(parent), timing(timing) {
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
    // timingIndex->setText(QString::number(index) + ":");
    timeEdit->setText(QString::number(timing->timestamp));
    timeEdit->setValidator(validator);

    auto this_cp = this;
    // 跳转按钮
    connect(gotoButton, &QPushButton::clicked,
            [this_cp]() { emit this_cp->gotoTiming(this_cp->timing); });
}
TimingParameterEditor::TimingParameterEditor(Timing *&timing,
                                             QTableWidget *parent,
                                             QDoubleValidator *validator)
    : QWidget() {
    // 1. 只创建一个主布局
    mainLayout = new QHBoxLayout(this);  // 直接设置给 this
    mainLayout->setSpacing(2);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // 2. 创建所有可能用到的控件
    bpmTitle = new QLabel("bpm:", this);
    bpmEdit = new QLineEdit(this);

    bpmEdit->setValidator(validator);
    bpmEdit->setText(QString::number(timing->bpm, 'f', 5));

    speedTitle = new QLabel(tr("speed:"), this);
    speedSpinBox = new QDoubleSpinBox(this);
    speedSpinBox->setDecimals(5);
    speedSpinBox->setSuffix("x");

    // 3. 将所有控件都添加到主布局中
    mainLayout->addWidget(bpmTitle);
    mainLayout->addWidget(bpmEdit);
    mainLayout->addWidget(speedTitle);
    mainLayout->addWidget(speedSpinBox);

    // 4. 根据条件切换控件的可见性
    if (timing->is_base_timing) {
        speedSpinBox->setValue(1.0);

        // 隐藏速度相关的控件
        speedTitle->hide();
        speedSpinBox->hide();

        // 显示BPM相关的控件 (确保它们是可见的)
        bpmTitle->show();
        bpmEdit->show();

    } else {
        speedSpinBox->setValue(100.0 / std::abs(timing->beat_length));

        // 隐藏BPM相关的控件
        bpmTitle->hide();
        bpmEdit->hide();

        // 显示速度相关的控件
        speedTitle->show();
        speedSpinBox->show();
    }
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

TimingRowItem::TimingRowItem(MMap *map, Timing *timing, QTableWidget *parent,
                             QIntValidator *intvalidator,
                             QDoubleValidator *doublevalidator) {
    this->timing = timing;
    timeEditWgt = new TimeEditWidget(this->timing, parent, intvalidator);
    uninheritedComboBox = new QComboBox(parent);
    uninheritedComboBox->addItems({"true", "false"});
    uninheritedComboBox->setCurrentIndex(timing->is_base_timing ? 0 : 1);
    paramEditor = new TimingParameterEditor(timing, parent, doublevalidator);
    timingSettingWgt = new TimingSettingWidget(timing, parent);

    // 切换继承
    connect(uninheritedComboBox, &QComboBox::currentIndexChanged,
            [this](int index) {
                // 切换参数编辑器的布局
                if (index == 0) {
                    // 隐藏速度相关的控件
                    paramEditor->speedTitle->hide();
                    paramEditor->speedSpinBox->hide();

                    // 显示BPM相关的控件
                    paramEditor->bpmTitle->show();
                    paramEditor->bpmEdit->show();
                }
                if (index == 1) {
                    // 隐藏BPM相关的控件
                    paramEditor->bpmTitle->hide();
                    paramEditor->bpmEdit->hide();

                    // 显示速度相关的控件
                    paramEditor->speedTitle->show();
                    paramEditor->speedSpinBox->show();
                }
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
