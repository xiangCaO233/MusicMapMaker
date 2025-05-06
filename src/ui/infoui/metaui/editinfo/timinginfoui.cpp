#include "timinginfoui.h"

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
    // 有选中内容时更新ui
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
