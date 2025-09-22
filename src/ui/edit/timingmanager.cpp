#include <AutoResizeTableWidget.h>
#include <timingmanager.h>
#include <ui_timingmanager.h>

#include <QStandardItemModel>

QStringList TimingManager::timing_metaNames() {
    return {tr("time"), tr("uninherited"), tr("parameter"), tr("setting")};
}

TimingManager::TimingManager(QWidget *parent)
    : QWidget(parent), ui(new Ui::TimingEditor) {
    ui->setupUi(this);

    // 初始化输入过滤器
    timeInputValidator = new QIntValidator(this);
    timeInputValidator->setBottom(0);
    parameterInputValidator = new QDoubleValidator(this);
    parameterInputValidator->setBottom(0.01);

    // 初始化timing表模型
    ui->timing_table_widget->setColumnCount(timing_metaNames().size());
    ui->timing_table_widget->setColumStretchs({1, 1, 1, 1});
    ui->timing_table_widget->setHorizontalHeaderLabels(timing_metaNames());
}

TimingManager::~TimingManager() { delete ui; }

void TimingManager::bind_toolcmdq(
    ThreadSafeQueue<ToolCommand> *tool_cmd_queue) {
    this->tool_cmd_queue = tool_cmd_queue;
}
