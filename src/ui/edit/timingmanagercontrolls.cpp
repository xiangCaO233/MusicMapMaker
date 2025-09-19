#include <qcombobox.h>
#include <qpushbutton.h>
#include <qwidget.h>
#include <timingmanager.h>
#include <ui_timingmanager.h>

#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpacerItem>
#include <cstdlib>
#include <mmm/map/MMap.hpp>
#include <mmm/timing/Timing.hpp>
#include <util/mutil.hpp>

// 时间编辑组件
class TimeEditWidget : public QWidget {
   public:
    TimeEditWidget(Timing *timing, QTableWidget *parent, int index,
                   QIntValidator *validator)
        : QWidget(parent), parent(parent) {
        layout = new QHBoxLayout(this);
        timingIndex = new QLabel(this);
        timeEdit = new QLineEdit(this);
        gotoButton = new QPushButton(this);
        QColor color = Qt::white;
        mutil::set_button_svgcolor(gotoButton, ":/icons/arrow-right.svg", color,
                                   16, 16);
        layout->addWidget(timingIndex);
        layout->addWidget(timeEdit);
        layout->addWidget(gotoButton);
        layout->setSpacing(2);
        layout->setContentsMargins(2, 0, 0, 2);
        layout->setStretch(0, 0);
        layout->setStretch(1, 1);
        layout->setStretch(2, 0);
        setLayout(layout);
        timingIndex->setText(QString::number(index) + ":");
        timeEdit->setText(QString::number(timing->timestamp));
        timeEdit->setValidator(validator);
    }
    QTableWidget *parent;
    QHBoxLayout *layout;
    QLabel *timingIndex;
    QLineEdit *timeEdit;
    QPushButton *gotoButton;
};

class TimingParameterEditor : public QWidget {
   public:
    TimingParameterEditor(Timing *timing, QTableWidget *parent,
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
    QHBoxLayout *layout;
    QString speedTitle = tr("speed:");
    QString bpmTitle = "bpm:";
    QLabel *title;
    QLineEdit *bpmEdit;
    QDoubleSpinBox *speedSpinBox;
};

// 时间点设置组件
class TimingSettingWidget : public QWidget {
   public:
    TimingSettingWidget(Timing *timing, QTableWidget *parent)
        : QWidget(parent) {
        layout = new QHBoxLayout(this);
        deleteButton = new QPushButton(this);
        QColor color = Qt::white;
        mutil::set_button_svgcolor(deleteButton, ":icons/close.svg", color, 16,
                                   16);

        doneButton = new QPushButton(this);
        mutil::set_button_svgcolor(doneButton, ":icons/check.svg", color, 16,
                                   16);
        spacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding,
                                 QSizePolicy::Policy::Minimum);
        layout->addItem(spacer);
        layout->addWidget(doneButton);
        layout->addWidget(deleteButton);
        setLayout(layout);
    };
    QTableWidget *parent;
    QHBoxLayout *layout;
    QSpacerItem *spacer;
    QPushButton *deleteButton;
    QPushButton *doneButton;
};

class TimingRowItem {
   public:
    TimingRowItem(Timing *timing, QTableWidget *parent, int index,
                  QIntValidator *intvalidator,
                  QDoubleValidator *doublevalidator) {
        timeEditWgt = new TimeEditWidget(timing, parent, index, intvalidator);
        uninheritedComboBox = new QComboBox(parent);
        uninheritedComboBox->addItems({"true", "false"});
        uninheritedComboBox->setCurrentIndex(timing->is_base_timing ? 0 : 1);
        paramEditor =
            new TimingParameterEditor(timing, parent, doublevalidator);
        timingSettingWgt = new TimingSettingWidget(timing, parent);
    }
    TimeEditWidget *timeEditWgt;
    QComboBox *uninheritedComboBox;
    TimingParameterEditor *paramEditor;
    TimingSettingWidget *timingSettingWgt;
};

void TimingManager::onTimingItemChanged(QStandardItem *item) {}

void TimingManager::onMapUpdated(MProject *project, MMap *map) {
    map_ref = map;
    ui->timing_table_widget->clear();
    if (map_ref) {
        qDebug() << "正在生成timing列表";
        // 读取map中所有timing导入timing表
        auto &timings = map_ref->timing_set().get_all_timing_points();
        for (const auto &[time, timing_vec] : timings) {
            for (const auto &timing : timing_vec) {
                // 获取新行的索引 (即当前的行数)
                int newRowIndex = ui->timing_table_widget->rowCount();
                // 在表格末尾插入一个新行
                ui->timing_table_widget->insertRow(newRowIndex);

                auto timingRow = new TimingRowItem(
                    timing.get(), ui->timing_table_widget, newRowIndex,
                    timeInputValidator, parameterInputValidator);

                // 时间
                ui->timing_table_widget->setCellWidget(newRowIndex, 0,
                                                       timingRow->timeEditWgt);

                // 继承
                ui->timing_table_widget->setCellWidget(
                    newRowIndex, 1, timingRow->uninheritedComboBox);

                // 参数
                ui->timing_table_widget->setCellWidget(newRowIndex, 2,
                                                       timingRow->paramEditor);

                // 设置
                ui->timing_table_widget->setCellWidget(
                    newRowIndex, 3, timingRow->timingSettingWgt);
            }
        }
        ui->timing_table_widget->setHorizontalHeaderLabels(timing_metaNames());
    }
}
