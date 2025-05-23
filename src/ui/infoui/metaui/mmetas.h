#ifndef MMETAS_H
#define MMETAS_H

#include <qtmetamacros.h>

#include <QWidget>
#include <memory>

class ObjectInfoui;
class TimingInfoui;
class TimeController;
class MMap;

enum class GlobalTheme;

namespace Ui {
class MMetas;
}

class MMetas : public QWidget {
    Q_OBJECT

   public:
    explicit MMetas(QWidget* parent = nullptr);
    ~MMetas();

    // 绑定的图
    std::shared_ptr<MMap> binding_map;
    bool table_sync_lock{false};

    // 标签页内组件引用
    ObjectInfoui* objinfo_ref;
    TimingInfoui* timinginfo_ref;
    TimeController* time_audio_controller_ref;

    // 当前主题
    GlobalTheme current_theme;

    // 使用主题
    void use_theme(GlobalTheme theme);

    static QStringList metaNames();
   public slots:
    void switch_map(std::shared_ptr<MMap> map);

   private slots:
    // 表格编辑完成
    void on_meta_table_cellChanged(int row, int column);

   private:
    Ui::MMetas* ui;
};

#endif  // MMETAS_H
