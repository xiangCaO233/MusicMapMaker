#include "newtimingguide.h"

#include <QDoubleValidator>
#include <QIntValidator>
#include <memory>

#include "../../mmm/map/MMap.h"
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
    if (!flag) {
        // 若锁了bpm编辑,解锁
        ui->bpm_edit->setEnabled(true);
    }
}

void NewTimingGuide::on_timestamp_edit_textChanged(const QString &arg1) {
    timestamp = arg1.toDouble();
    // 查询谱面的在当前time之前有无非继承时间点
    // 据此设置继承按钮的可用性

    if (map) {
        if (map->temp_timing_map.empty())
            inheritable = false;
        else {
            // 第一个大于等于当前时间的时间点列表
            auto it = map->temp_timing_map.lower_bound(int(timestamp));
            if (it == map->temp_timing_map.end()) {
                // 在此之后无其他时间点
                // 向前移动
                --it;
                while (it != map->temp_timing_map.begin() &&
                       it->second.at(0)->timestamp > timestamp) {
                    --it;
                }
                if (timestamp == it->second.at(0)->timestamp) {
                    if (it->second.at(0)->is_base_timing &&
                        it->second.size() == 1) {
                        // 是绝对bpm且无同时变速时间点,可添加
                        inheritable = true;
                        inheritable_timing = it->second.at(0);
                    }
                } else {
                    // 有严格小于当前时间的时间点
                    inheritable = true;
                    // 取那个时间的绝对时间点
                    for (const auto &timing : it->second) {
                        if (timing->is_base_timing) {
                            inheritable_timing = timing;
                            break;
                        }
                    }
                }
            } else if (it == map->temp_timing_map.begin()) {
                // 查到第一个了
                if (it->second.at(0)->is_base_timing &&
                    it->second.size() == 1) {
                    // 是绝对bpm且无同时变速时间点,可添加
                    inheritable = true;
                    inheritable_timing = it->second.at(0);
                } else {
                    inheritable = false;
                }
            } else {
                // 查到中间
                if (timestamp == it->second.at(0)->timestamp) {
                    // 查到当前的
                    if (it->second.at(0)->is_base_timing &&
                        it->second.size() == 1) {
                        // 是绝对bpm且无同时变速时间点,可添加
                        inheritable = true;
                        inheritable_timing = it->second.at(0);
                    } else {
                        inheritable = false;
                    }
                } else {
                    // 查到之后的
                    // 向前移动
                    while (it != map->temp_timing_map.begin() &&
                           it->second.at(0)->timestamp > timestamp) {
                        --it;
                    }
                    if (timestamp == it->second.at(0)->timestamp) {
                        if (it->second.at(0)->is_base_timing &&
                            it->second.size() == 1) {
                            // 是绝对bpm且无同时变速时间点,可添加
                            inheritable = true;
                            inheritable_timing = it->second.at(0);
                        }
                    } else {
                        // 有严格小于当前时间的时间点
                        inheritable = true;
                        // 取那个时间的绝对时间点
                        for (const auto &timing : it->second) {
                            if (timing->is_base_timing) {
                                inheritable_timing = timing;
                                break;
                            }
                        }
                    }
                }
            }
        }
        // 根据是否可以继承更新按钮状态
        set_inheritable(inheritable);
    }
}

void NewTimingGuide::on_bpm_edit_textChanged(const QString &arg1) {
    bpm = arg1.toDouble();
}

void NewTimingGuide::on_play_speed_edit_textChanged(const QString &arg1) {
    speed = arg1.toDouble();
}

void NewTimingGuide::on_inheritable_checkbox_checkStateChanged(
    const Qt::CheckState &arg1) {
    // TODO(xiang 2025-05-16): 切换时间点继承状态
    if (arg1) {
        // 锁定bpm
        ui->bpm_edit->setText(
            QString::number(inheritable_timing->basebpm, 'f', 2));
        ui->bpm_edit->setEnabled(false);
    } else {
        ui->bpm_edit->setEnabled(true);
    }
    inheritance_pretiming = arg1;
}

void NewTimingGuide::on_enable_speed_changing_checkStateChanged(
    const Qt::CheckState &arg1) {
    ui->play_speed_edit->setEnabled(arg1);
    ui->adapt_to_preferencebpm->setEnabled(arg1);
}

void NewTimingGuide::on_adapt_to_preferencebpm_clicked() {
    // TODO(xiang 2025-05-16):
    // 根据map的参考bpm计算当前bpm保持播放速度需要的变速值
}
