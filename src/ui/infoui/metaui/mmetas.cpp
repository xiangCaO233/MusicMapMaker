#include "mmetas.h"

#include "../../../mmm/hitobject/HitObject.h"
#include "../../../mmm/timing/Timing.h"
#include "ui_mmetas.h"

MMetas::MMetas(QWidget* parent) : QWidget(parent), ui(new Ui::MMetas) {
  ui->setupUi(this);
}

MMetas::~MMetas() { delete ui; }

// 画布选中物件事件
void MMetas::on_canvas_select_object(std::shared_ptr<HitObject>& obj,
                                     std::shared_ptr<Timing>& ref_timing) {
  // 更新标签信息显示
  // if (obj) {
  //   ui->timestam_value->setText(QString::number(obj->timestamp, 'f', 0));
  // }
  // if (ref_timing) {
  //   ui->ntiming_bpm_value->setText(
  //       QString::number(ref_timing->basebpm, 'f', 2));
  // }
}
