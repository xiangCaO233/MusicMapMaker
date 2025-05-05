#ifndef MINFO_H
#define MINFO_H

#include <QWidget>

#include "../log/loglevel.h"

class MMetas;
enum class GlobalTheme;

namespace Ui {
class MInfo;
}

class MInfo : public QWidget {
  Q_OBJECT

 public:
  explicit MInfo(QWidget *parent = nullptr);
  ~MInfo();
  MMetas *mmetas;

  // 当前主题
  GlobalTheme current_theme;

  // 使用主题
  void use_theme(GlobalTheme theme);

  // 输出日志
  void log(MLogLevel level, const QString &message);

 private:
  Ui::MInfo *ui;
};

#endif  // MINFO_H
