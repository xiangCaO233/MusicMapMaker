#include "timinginfoui.h"

#include "../../../../mmm/timing/Timing.h"
#include "../../GlobalSettings.h"
#include "../../util/mutil.h"
#include "ui_timinginfoui.h"

TimingInfoui::TimingInfoui(QWidget *parent)
    : QWidget(parent), ui(new Ui::TimingInfoui) {
  ui->setupUi(this);
  update_selected_uiinfo();
}

TimingInfoui::~TimingInfoui() { delete ui; }

// 使用主题
void TimingInfoui::use_theme(GlobalTheme theme) {
  QColor file_button_color;
  switch (theme) {
    case GlobalTheme::DARK: {
      file_button_color = QColor(255, 255, 255);
      break;
    }
    case GlobalTheme::LIGHT: {
      file_button_color = QColor(0, 0, 0);
      break;
    }
  }
  // mutil::set_button_svgcolor(ui->goto_timing_button,
  // ":/icons/angle-right.svg",
  //                            file_button_color, 16, 16);
}

// 画布选中物件事件
void TimingInfoui::on_canvas_select_timings(
    std::vector<std::shared_ptr<Timing>> *timings) {
  current_select_timings = timings;
  update_selected_uiinfo();
}

// 更新选中的ui信息
void TimingInfoui::update_selected_uiinfo() {
  if (current_select_timings) {
    if (current_select_timings->empty()) return;
    // 有选中内容时更新ui
    ui->timestamp_edit->setText(
        QString::number(current_select_timings->at(0)->timestamp, 'f', 0));

    // 检查变速
    bool show_speed{false};
    if (current_select_timings->size() >= 2) {
      show_speed = true;
      ui->speed_value_lineEdit->setText(
          QString::number(current_select_timings->at(1)->bpm, 'f', 2));
    } else {
      if (!current_select_timings->at(0)->is_base_timing) {
        // 就一个-看这个是不是变速
        show_speed = true;
        ui->speed_value_lineEdit->setText(
            QString::number(current_select_timings->at(0)->bpm, 'f', 2));
      }
    }
    ui->bpm_value_lineEdit->setText(
        QString::number(current_select_timings->at(0)->basebpm, 'f', 2));

    // 是否展示变速
    if (show_speed) {
      ui->speed_edit_container->setHidden(false);
    } else {
      ui->speed_edit_container->hide();
    }

    if (isHidden()) {
      setHidden(false);
    }
  } else {
    // 无选中时隐藏组件
    if (!isHidden()) {
      hide();
    }
  }
}
