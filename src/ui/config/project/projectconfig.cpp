#include <config/project/projectconfig.h>
#include <ui_projectconfig.h>

ProjectConfig::ProjectConfig(QWidget *parent)
    : QWidget(parent), ui(new Ui::ProjectConfig) {
    ui->setupUi(this);
    hideConfigs();
}

ProjectConfig::~ProjectConfig() { delete ui; }

// 隐藏配置组件
void ProjectConfig::hideConfigs() { ui->scrollAreaWidgetContents->hide(); }

// 显示配置组件
void ProjectConfig::showConfigs() const {
    ui->scrollAreaWidgetContents->show();
}

// 绑定配置
void ProjectConfig::bind_config(MProjectConfig *cfg) const {
    config = cfg;
    showConfigs();
}
