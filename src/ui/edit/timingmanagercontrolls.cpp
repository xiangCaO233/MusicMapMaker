#include <timingmanager.h>
#include <ui_timingmanager.h>

#include <cstdlib>
#include <mmm/map/MMap.hpp>
#include <mmm/timing/Timing.hpp>

void TimingManager::onTimingItemChanged(QStandardItem *item) {}

void TimingManager::onMapUpdated(MProject *project, MMap *map) {
    map_ref = map;
    ui->timing_table_widget->clear();
    if (map_ref) {
        // 读取map中所有timing导入timing表
        auto &timings = map_ref->timing_set().get_all_timing_points();
        for (const auto &[time, timing_vec] : timings) {
            for (const auto &timing : timing_vec) {
                // 获取新行的索引 (即当前的行数)
                int newRowIndex = ui->timing_table_widget->rowCount();
                // 在表格末尾插入一个新行
                ui->timing_table_widget->insertRow(newRowIndex);
                // 时间
                ui->timing_table_widget->setItem(
                    newRowIndex, 0,
                    new QTableWidgetItem(QString::number(time)));
                // 继承
                ui->timing_table_widget->setItem(
                    newRowIndex, 1,
                    new QTableWidgetItem(timing->is_base_timing ? "true"
                                                                : "false"));
                // 参数
                ui->timing_table_widget->setItem(
                    newRowIndex, 2,
                    new QTableWidgetItem(QString::number(
                        timing->is_base_timing
                            ? timing->bpm
                            : 100. / std::abs(timing->beat_length))));
            }
        }
        ui->timing_table_widget->setHorizontalHeaderLabels(timing_metaNames());
    }
}
