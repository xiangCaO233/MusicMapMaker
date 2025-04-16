#include "projectcontroller.h"

#include "ui_mprojectcontroller.h"

MProjectController::MProjectController(QWidget* parent)
    : QWidget(parent), ui(new Ui::MProjectController) {
  ui->setupUi(this);
}

MProjectController::~MProjectController() { delete ui; }

// 新项目槽函数
void MProjectController::new_project(std::shared_ptr<MapWorkProject>& project) {

}
