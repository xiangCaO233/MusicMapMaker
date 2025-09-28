#ifndef PROJECTCONFIG_H
#define PROJECTCONFIG_H

#include <QWidget>

namespace Ui {
class ProjectConfig;
}

class MProject;
class MProjectConfig;

class ProjectConfig : public QWidget {
    Q_OBJECT

   public:
    explicit ProjectConfig(QWidget *parent = nullptr);
    ~ProjectConfig() override;

    // 隐藏配置组件
    void hideConfigs();

    // 显示配置组件
    void showConfigs() const;

   public slots:
    // 绑定配置
    void bind_config(MProject *cfg);

   signals:
    void layout_changed();
    void judgeline_changed(float pos);

   private slots:
    void on_toppos_spinner_valueChanged(double arg1);

    void on_rightpos_spinner_valueChanged(double arg1);

    void on_bottompos_spinner_valueChanged(double arg1);

    void on_leftpos_spinner_valueChanged(double arg1);

    void on_object_width_scale_slider_valueChanged(int value);

    void on_object_height_scale_slider_valueChanged(int value);

    void on_judgeline_pos_slider_valueChanged(int value);

    void on_project_name_edit_textEdited(const QString &arg1);

   private:
    Ui::ProjectConfig *ui;

    // 绑定的配置
    mutable MProjectConfig *config;
};

#endif  // PROJECTCONFIG_H
