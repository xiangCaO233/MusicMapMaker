#ifndef MMETAS_H
#define MMETAS_H

#include <qtmetamacros.h>

#include <QWidget>
#include <memory>

class Timing;
class HitObject;
class Beat;
enum class GlobalTheme;

namespace Ui {
class MMetas;
}

class MMetas : public QWidget {
  Q_OBJECT

 public:
  explicit MMetas(QWidget* parent = nullptr);
  ~MMetas();

  // 当前主题
  GlobalTheme current_theme;

  // 使用主题
  void use_theme(GlobalTheme theme);

 public slots:
  // 画布选中物件事件
  void on_canvas_select_object(std::shared_ptr<Beat> beatinfo,
                               std::shared_ptr<HitObject> obj,
                               std::shared_ptr<Timing> ref_timing);

 private:
  Ui::MMetas* ui;
};

#endif  // MMETAS_H
