#ifndef MMETAS_H
#define MMETAS_H

#include <qtmetamacros.h>

#include <QWidget>
#include <memory>

class Timing;
class HitObject;

namespace Ui {
class MMetas;
}

class MMetas : public QWidget {
  Q_OBJECT

 public:
  explicit MMetas(QWidget* parent = nullptr);
  ~MMetas();

 public slots:
  // 画布选中物件事件
  void on_canvas_select_object(std::shared_ptr<HitObject>& obj,
                               std::shared_ptr<Timing>& ref_timing);

 private:
  Ui::MMetas* ui;
};

#endif  // MMETAS_H
