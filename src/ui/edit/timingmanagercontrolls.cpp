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
#include <TimingTableUsefulWidgets.hpp>
#include <cstdlib>
#include <mmm/map/MMap.hpp>
#include <mmm/map/editor/MMapEditor.hpp>
#include <mmm/timing/Timing.hpp>
#include <util/mutil.hpp>
#include <utility>

auto addTimingItemToTable(MMap *map, Timing *timing,
                          QTableWidget *timing_table_widget, int row_index,
                          QIntValidator *intvalidator,
                          QDoubleValidator *doublevalidator) {
    // 在表格末尾插入一个新行
    timing_table_widget->insertRow(row_index);

    auto timingRow =
        new TimingRowItem(map, timing, timing_table_widget, row_index,
                          intvalidator, doublevalidator);

    // 时间
    timing_table_widget->setCellWidget(row_index, 0, timingRow->timeEditWgt);

    // 继承
    timing_table_widget->setCellWidget(row_index, 1,
                                       timingRow->uninheritedComboBox);

    // 参数
    timing_table_widget->setCellWidget(row_index, 2, timingRow->paramEditor);

    // 设置
    timing_table_widget->setCellWidget(row_index, 3,
                                       timingRow->timingSettingWgt);
    return timingRow;
}

void TimingManager::onMapUpdated(MProject *project, MMap *map) {
    if (map_ref == map) return;
    map_ref = map;
    ui->timing_table_widget->setRowCount(0);
    if (map_ref) {
        qDebug() << "正在生成timing列表";
        // 读取map中所有timing导入timing表
        auto &timings = map_ref->timing_set().get_all_timing_points();
        for (const auto &[time, timing_vec] : timings) {
            for (const auto &timing : timing_vec) {
                // 获取新行的索引 (即当前的行数)
                int newRowIndex = ui->timing_table_widget->rowCount();
                auto item = addTimingItemToTable(
                    map, timing.get(), ui->timing_table_widget, newRowIndex,
                    timeInputValidator, parameterInputValidator);
                auto timing_ptr = timing.get();
            }
        }

        auto addingItem = new AddTimingItem(ui->timing_table_widget);
        // 获取新行的索引 (即当前的行数)
        int newRowIndex = ui->timing_table_widget->rowCount();
        // 在表格末尾插入一个新行
        ui->timing_table_widget->insertRow(newRowIndex);
        // 设置
        ui->timing_table_widget->setCellWidget(newRowIndex, 3,
                                               addingItem->setting_widget);

        auto timing_table_ref = ui->timing_table_widget;
        QIntValidator *intvalidator = timeInputValidator;
        QDoubleValidator *doublevalidator = parameterInputValidator;

        // 添加timing按键
        connect(addingItem->insertButton, &QPushButton::clicked,
                [map, timing_table_ref, intvalidator, doublevalidator]() {
                    auto timing = std::make_unique<Timing>();
                    timing->timestamp = map->base_metadata().map_length;
                    timing->bpm = 60;
                    timing->is_base_timing = true;
                    timing->beat_length = 1000;
                    auto item = addTimingItemToTable(
                        map, timing.get(), timing_table_ref,
                        timing_table_ref->rowCount() - 1, intvalidator,
                        doublevalidator);
                    map->editor()->creatTiming(std::move(timing));
                });

        ui->timing_table_widget->setHorizontalHeaderLabels(timing_metaNames());
        qDebug() << "生成timing列表完成";
    }
}
