#include <AutoResizeTableWidget.h>
#include <timingmanager.h>
#include <ui_timingmanager.h>

#include <QStandardItemModel>
#include <mmm/timing/Timing.hpp>

QStringList TimingManager::timing_metaNames() {
    return {tr("time"), tr("uninherited"), tr("parameter")};
}

TimingManager::TimingManager(QWidget *parent)
    : QWidget(parent), ui(new Ui::TimingEditor) {
    ui->setupUi(this);

    // 初始化timing表模型
    auto timing_table_model = new QStandardItemModel(ui->timing_table_widget);
    ui->timing_table_widget->setColumnCount(timing_metaNames().size());
    ui->timing_table_widget->setHorizontalHeaderLabels(timing_metaNames());
    ui->timing_table_widget->setColumStretchs({1, 1, 1});
    // 连接模型信号到项改变槽
    connect(timing_table_model, &QStandardItemModel::itemChanged, this,
            &TimingManager::onTimingItemChanged);
}

TimingManager::~TimingManager() { delete ui; }
