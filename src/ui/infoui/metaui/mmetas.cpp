#include "mmetas.h"

#include <memory>

#include "../../../mmm/hitobject/HitObject.h"
#include "../../../mmm/hitobject/Note/Hold.h"
#include "../../../mmm/hitobject/Note/HoldEnd.h"
#include "../../../mmm/timing/Timing.h"
#include "../../GlobalSettings.h"
#include "../../util/mutil.h"
#include "mmm/Beat.h"
#include "ui_mmetas.h"

MMetas::MMetas(QWidget* parent) : QWidget(parent), ui(new Ui::MMetas) {
  ui->setupUi(this);
}

MMetas::~MMetas() { delete ui; }

// 使用主题
void MMetas::use_theme(GlobalTheme theme) {
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
  mutil::set_button_svgcolor(ui->goto_timing_button, ":/icons/angle-right.svg",
                             file_button_color, 16, 16);
}

// 画布选中物件事件
void MMetas::on_canvas_select_object(std::shared_ptr<Beat> beatinfo,
                                     std::shared_ptr<HitObject> obj,
                                     std::shared_ptr<Timing> ref_timing) {
  // 更新标签信息显示
  if (obj) {
    // 更新类型标签
    QString typev;
    switch (obj->object_type) {
      case HitObjectType::NOTE: {
        typev = tr("Note");
        break;
      }
      case HitObjectType::HOLDEND:
      case HitObjectType::OSUHOLDEND: {
        return;
      }
      case HitObjectType::HOLD:
      case HitObjectType::OSUHOLD: {
        typev = tr("Hold");
        break;
      }
      case HitObjectType::RMSLIDE: {
        typev = tr("Slide");
        break;
      }
      default: {
        typev = tr("Unknown");
        break;
      }
    }
    ui->type_value->setText(typev);

    // 更新物件时间戳标签
    if (obj->is_note) {
      auto note = std::static_pointer_cast<Note>(obj);
      if (note->note_type == NoteType::HOLD) {
        Hold* hold = std::static_pointer_cast<Hold>(note).get();
        ui->timestam_value->setText(
            QString::number(hold->timestamp, 'f', 0) + "\n" + tr("Hold time:") +
            QString::number(hold->hold_time, 'f', 0) + "ms");
      } else {
        // 其他
        ui->timestam_value->setText(QString::number(obj->timestamp, 'f', 0));
      }
      // 更新轨道信息
      ui->orbit_value->setText(QString::number(note->orbit + 1));
    }
    // 更新分拍位置信息
    if (beatinfo) {
      ui->divisor_value->setText(QString::number(obj->divpos) + "/" +
                                 QString::number(beatinfo->divisors));
    } else {
      ui->divisor_value->setText("0/1");
    }
  } else {
    ui->type_value->setText("");
    ui->timestam_value->setText("");
    ui->divisor_value->setText("");
    ui->orbit_value->setText("");
  }

  if (ref_timing) {
    // 更新timing标签
    ui->ntiming_time_value->setText(
        QString::number(ref_timing->timestamp, 'f', 0));
    ui->ntiming_bpm_value->setText(
        QString::number(ref_timing->basebpm, 'f', 2));
  } else {
    ui->ntiming_time_value->setText("");
    ui->ntiming_bpm_value->setText("");
  }
}
