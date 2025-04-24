#ifndef MPAGE_H
#define MPAGE_H

#include <qtmetamacros.h>

#include <QWidget>
#include <memory>

#include "../../GlobalSettings.h"

class MMap;
class XAudioManager;
class MEditorArea;

namespace Ui {
class MPage;
}

class MPage : public QWidget {
  Q_OBJECT

 public:
  explicit MPage(QWidget *parent = nullptr);
  ~MPage();

  // 编辑区引用
  MEditorArea *edit_area_widget;

  // 音频管理器
  std::shared_ptr<XAudioManager> audio_manager_reference;

  // page名的谱面映射表
  std::unordered_map<QString, std::shared_ptr<MMap>> pagetext_maps_map;

  // 使用主题
  void use_theme(GlobalTheme theme);
 signals:
  // 选择map信号
  void select_map(std::shared_ptr<MMap> &map);

 public slots:
  // 项目控制器选择了map事件
  void project_controller_select_map(std::shared_ptr<MMap> &map);

 private slots:
  // 选择页事件
  void on_page_selector_currentTextChanged(const QString &arg1);

  // 关闭页面事件
  void on_close_page_button_clicked();

 private:
  Ui::MPage *ui;
};

#endif  // MPAGE_H
