#ifndef PROJECTCONFIG_H
#define PROJECTCONFIG_H

#include <QWidget>

namespace Ui {
class ProjectConfig;
}

class MProjectConfig;

class ProjectConfig : public QWidget {
    Q_OBJECT

   public:
    explicit ProjectConfig(QWidget *parent = nullptr);
    ~ProjectConfig() override;

    // 绑定配置
    void bind_config(MProjectConfig *cfg);

   private:
    Ui::ProjectConfig *ui;

    // 绑定的配置
    MProjectConfig *config;
};

#endif  // PROJECTCONFIG_H
