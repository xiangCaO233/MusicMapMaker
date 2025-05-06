#ifndef MMETAS_H
#define MMETAS_H

#include <qtmetamacros.h>

#include <QWidget>
#include <memory>

class ObjectInfoui;
class TimingInfoui;

enum class GlobalTheme;

namespace Ui {
class MMetas;
}

class MMetas : public QWidget {
  Q_OBJECT

 public:
  explicit MMetas(QWidget* parent = nullptr);
  ~MMetas();

  // 标签页内组件引用
  ObjectInfoui* objinfo_ref;
  TimingInfoui* timinginfo_ref;

  // 当前主题
  GlobalTheme current_theme;

  // 使用主题
  void use_theme(GlobalTheme theme);

 private:
  Ui::MMetas* ui;
};

#endif  // MMETAS_H
