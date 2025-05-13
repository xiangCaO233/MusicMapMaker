#ifndef OBJECTINFOUI_H
#define OBJECTINFOUI_H

#include <QWidget>

class Timing;
class HitObject;
class Beat;

enum class GlobalTheme;

namespace Ui {
class ObjectInfoui;
}

class ObjectInfoui : public QWidget {
    Q_OBJECT

   public:
    explicit ObjectInfoui(QWidget *parent = nullptr);
    ~ObjectInfoui();

    // 当前选中的物件信息
    std::shared_ptr<Beat> current_beatinfo{nullptr};
    std::shared_ptr<HitObject> current_obj{nullptr};
    std::shared_ptr<Timing> current_ref_timing{nullptr};

    // 当前主题
    GlobalTheme current_theme;

    // 使用主题
    void use_theme(GlobalTheme theme);

    // 更新选中的ui信息
    void update_selected_uiinfo();

   public slots:
    // 画布选中物件事件
    void on_canvas_select_object(std::shared_ptr<Beat> beatinfo,
                                 std::shared_ptr<HitObject> obj,
                                 std::shared_ptr<Timing> ref_timing);

   private:
    Ui::ObjectInfoui *ui;
};

#endif  // OBJECTINFOUI_H
