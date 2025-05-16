#include "newtimingguide.h"

#include <QDoubleValidator>
#include <QIntValidator>

#include "ui_newtimingguide.h"

NewTimingGuide::NewTimingGuide(std::shared_ptr<MMap> binding_map,
                               QWidget *parent)
    : QDialog(parent), map(binding_map), ui(new Ui::NewTimingGuide) {
    ui->setupUi(this);
    // double mask
    auto doubleValidator = new QDoubleValidator(this);
    doubleValidator->setNotation(QDoubleValidator::StandardNotation);
    ui->bpm_edit->setValidator(doubleValidator);
    ui->play_speed_edit->setValidator(doubleValidator);

    // 纯数字 mask
    auto intValidator = new QIntValidator(this);
    ui->timestamp_edit->setValidator(intValidator);
}

NewTimingGuide::~NewTimingGuide() { delete ui; }

void NewTimingGuide::set_time(double time) {
    timestamp = time;
    ui->timestamp_edit->setText(QString::number(time, 'f', 0));
}

void NewTimingGuide::set_inheritable(bool flag) {
    inheritable = flag;
    ui->inheritable_checkbox->setEnabled(flag);
    if (!flag && !ui->bpm_edit->isEnabled()) {
        // 若锁了bpm编辑,解锁
        ui->bpm_edit->setEnabled(true);
    }
}

void NewTimingGuide::on_timestamp_edit_textChanged(const QString &arg1) {
    timestamp = arg1.toDouble();
}

void NewTimingGuide::on_bpm_edit_textChanged(const QString &arg1) {
    bpm = arg1.toDouble();
}

void NewTimingGuide::on_play_speed_edit_textChanged(const QString &arg1) {
    speed = arg1.toDouble();
}

void NewTimingGuide::on_inheritable_checkbox_checkStateChanged(
    const Qt::CheckState &arg1) {}

void NewTimingGuide::on_enable_speed_changing_checkStateChanged(
    const Qt::CheckState &arg1) {}

void NewTimingGuide::on_adapt_to_preferencebpm_clicked() {}
