#ifndef TIMINGINFOUI_H
#define TIMINGINFOUI_H

#include <QWidget>
#include <memory>
#include <vector>

class Timing;

enum class GlobalTheme;

namespace Ui {
class TimingInfoui;
}

class TimingInfoui : public QWidget {
    Q_OBJECT

   public:
    explicit TimingInfoui(QWidget *parent = nullptr);
    ~TimingInfoui();

    // 当前选中的timings裸指
    std::vector<std::shared_ptr<Timing>> *current_select_timings{nullptr};

    // 当前主题
    GlobalTheme current_theme;

    // 使用主题
    void use_theme(GlobalTheme theme);

    // 更新选中的ui信息
    void update_selected_uiinfo();

   public slots:
    // 画布选中物件事件
    void on_canvasSelectTimings(std::vector<std::shared_ptr<Timing>> *timings);

   private:
    Ui::TimingInfoui *ui;
};

#endif  // TIMINGINFOUI_H
