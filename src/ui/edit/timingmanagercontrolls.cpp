#include <log/colorful-log.h>
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
#include <mmm/map/MMap.hpp>
#include <mmm/map/editor/MMapEditor.hpp>
#include <mmm/timing/Timing.hpp>
#include <util/mutil.hpp>
#include <utility>

void TimingManager::onMapUpdated(MProject* project, MMap* map) {
    if (map_ref == map) return;

    // 如果存在旧的map，断开与其编辑器的连接
    if (map_ref) {
        auto editor = map_ref->editor();
        disconnect(editor.get(), &MMapEditor::timingMapUpdated, this,
                   &TimingManager::refreshTableFromMap);
    }

    map_ref = map;

    // 连接到新map的编辑器
    if (map_ref) {
        auto editor = map_ref->editor();
        connect(editor.get(), &MMapEditor::timingMapUpdated, this,
                &TimingManager::refreshTableFromMap);
    }

    // 触发对表格的首次、完全的刷新
    refreshTableFromMap();
}

// 添加一个新timing行
void TimingManager::addNewTimingRowItem(MMap* map, Timing* newTiming) {
    // 创建行
    auto item = new TimingRowItem(map, newTiming, ui->timing_table_widget,
                                  timeInputValidator, parameterInputValidator);
    allTimingRowItems.append(item);

    auto this_cp = this;
    // 跳转到timing点按钮
    connect(
        item->timeEditWgt, &TimeEditWidget::gotoTiming,
        [this_cp](Timing* timing) { emit this_cp->navigateToTiming(timing); });

    // 完成编辑按钮和删除按钮
    connect(
        item->timingSettingWgt->doneButton, &QPushButton::clicked,
        [this_cp, map, item]() {
            // 检查更新
            auto desTime = item->timeEditWgt->timeEdit->text().toInt();
            auto des_is_base_timing =
                item->uninheritedComboBox->currentIndex() == 0;
            auto des_param =
                des_is_base_timing
                    ? item->paramEditor->bpmEdit->text().toDouble()
                    : -100.0 / item->paramEditor->speedSpinBox->value();
            auto param_same =
                item->timing->is_base_timing
                    ? item->timing->bpm == des_param
                    : 100.0 / std::abs(item->timing->beat_length) == des_param;
            if (desTime == item->timing->timestamp &&
                des_is_base_timing == item->timing->is_base_timing &&
                param_same) {
            } else {
                // 发送更新指令
                auto newTiming = item->timing->clone();
                newTiming->timestamp = desTime;
                newTiming->is_base_timing = des_is_base_timing;
                newTiming->bpm =
                    des_is_base_timing ? des_param : item->timing->bpm;
                newTiming->beat_length =
                    des_is_base_timing ? 60000.0 / newTiming->bpm : des_param;
                auto newTimingPtr = newTiming.get();
                map->editor()->updateTiming(item->timing, std::move(newTiming));
                // 更新本地指针
                item->timing = newTimingPtr;
            }
        });

    // 删除按钮
    connect(item->timingSettingWgt->deleteButton, &QPushButton::clicked,
            [item, map]() {
                // 删除当前timing
                map->editor()->deleteTiming(item->timing);
            });
}

// 从map刷新timing表
void TimingManager::refreshTableFromMap() {
    XINFO("1/3: 开始从 Map 刷新...");

    // --- 清理所有旧的UI状态 ---
    qDeleteAll(allTimingRowItems);
    allTimingRowItems.clear();
    delete addTimingItem;
    addTimingItem = nullptr;
    ui->timing_table_widget->blockSignals(true);  // 阻塞信号，防止不必要的更新
    ui->timing_table_widget->setRowCount(0);

    if (!map_ref) {
        XWARN("刷新中止，因为没有 map。");
        ui->timing_table_widget->blockSignals(false);
        return;
    }

    // --- 从 MMap 重新加载数据到 allTimingRowItems ---
    auto& timings = map_ref->timing_set().get_all_timing_points();
    for (const auto& [time, timing_vec] : timings) {
        for (const auto& timing : timing_vec) {
            addNewTimingRowItem(map_ref, timing.get());
        }
    }

    // --- 调用简化的排序和渲染函数 ---
    sortAndRebuildTable();

    // --- 在渲染完成后，处理“添加”行 ---
    addTimingItem = new AddTimingItem(ui->timing_table_widget);
    int addRowIndex = ui->timing_table_widget->rowCount();
    ui->timing_table_widget->insertRow(addRowIndex);
    ui->timing_table_widget->setCellWidget(addRowIndex, 3,
                                           addTimingItem->setting_widget);

    connect(addTimingItem->insertButton, &QPushButton::clicked, this, [this]() {
        if (!map_ref) return;
        auto timing = std::make_unique<Timing>();
        timing->timestamp = map_ref->base_metadata().map_length;
        timing->bpm = 60;
        timing->is_base_timing = true;
        timing->beat_length = 1000;
        map_ref->editor()->creatTiming(std::move(timing));
    });

    // --- 所有UI操作完成后，恢复信号并设置表头 ---
    ui->timing_table_widget->setHorizontalHeaderLabels(timing_metaNames());
    ui->timing_table_widget->blockSignals(false);

    XINFO("3/3: 刷新全部完成。");
}

void TimingManager::sortAndRebuildTable() {
    XINFO("2/3: 正在排序并渲染 Timing 表...");

    // 对“UI组件管理器”列表进行排序
    std::sort(allTimingRowItems.begin(), allTimingRowItems.end(),
              [](const TimingRowItem* a, const TimingRowItem* b) {
                  // --- 优先级 1: 按时间戳升序排序 ---
                  if (a->timing->timestamp != b->timing->timestamp) {
                      return a->timing->timestamp < b->timing->timestamp;
                  }

                  // --- 优先级 2: 如果时间戳相同，按 ComboBox 的索引排序 ---
                  // 规则: 选项0 排在 选项1 上面。
                  // ComboBox 的 currentIndex() 分别返回 0 和 1。
                  // a 的索引 < b 的索引 (0 < 1) 意味着 a 应该排在前面。
                  // 这正好是标准的升序排序。
                  return a->uninheritedComboBox->currentIndex() <
                         b->uninheritedComboBox->currentIndex();
              });

    // 步骤2：根据已排序的列表，填充表格
    // 注意：我们不再需要自己清空表格 (setRowCount(0))，
    // 因为调用者 refreshTableFromMap 已经做过了。
    // 我们也不需要 blockSignals，因为 refreshTableFromMap 也可以处理。

    for (int i = 0; i < allTimingRowItems.size(); ++i) {
        TimingRowItem* item = allTimingRowItems.at(i);

        // 插入新行
        ui->timing_table_widget->insertRow(i);

        // 更新索引标签的文本 (这是这个函数的核心价值之一)
        item->timeEditWgt->timingIndex->setText(QString::number(i) + ":");

        // 将该 item 拥有的控件设置到新的行中
        ui->timing_table_widget->setCellWidget(i, 0, item->timeEditWgt);
        ui->timing_table_widget->setCellWidget(i, 1, item->uninheritedComboBox);
        ui->timing_table_widget->setCellWidget(i, 2, item->paramEditor);
        ui->timing_table_widget->setCellWidget(i, 3, item->timingSettingWgt);
    }

    // 设置表头的职责也可以移交给 refreshTableFromMap，以保持单一职责
    // ui->timing_table_widget->setHorizontalHeaderLabels(timing_metaNames());

    XINFO("排序和渲染完成。");
}
