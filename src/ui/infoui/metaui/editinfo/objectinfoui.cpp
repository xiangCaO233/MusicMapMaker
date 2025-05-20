#include "objectinfoui.h"

#include "../../../mmm/hitobject/HitObject.h"
#include "../../../mmm/hitobject/Note/Hold.h"
#include "../../../mmm/timing/Timing.h"
#include "../../GlobalSettings.h"
#include "../../util/mutil.h"
#include "mmm/Beat.h"
#include "ui_objectinfoui.h"

ObjectInfoui::ObjectInfoui(QWidget* parent)
    : QWidget(parent), ui(new Ui::ObjectInfoui) {
    ui->setupUi(this);
    update_selected_uiinfo();
}

ObjectInfoui::~ObjectInfoui() { delete ui; }

// 使用主题
void ObjectInfoui::use_theme(GlobalTheme theme) {
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
    mutil::set_button_svgcolor(ui->goto_timing_button,
                               ":/icons/angle-right.svg", file_button_color, 16,
                               16);
}

// 画布选中物件事件
void ObjectInfoui::on_canvas_select_object(Beat* beatinfo,
                                           std::shared_ptr<HitObject> obj,
                                           std::shared_ptr<Timing> ref_timing) {
    current_obj = obj;
    current_beatinfo = beatinfo;
    current_ref_timing = ref_timing;
    update_selected_uiinfo();
}

// 更新选中的ui信息
void ObjectInfoui::update_selected_uiinfo() {
    // 更新标签信息显示
    if (current_obj) {
        // 更新类型标签
        QString typev;
        switch (current_obj->object_type) {
            case HitObjectType::NOTE:
            case HitObjectType::OSUNOTE: {
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
        if (current_obj->is_note) {
            auto note = std::static_pointer_cast<Note>(current_obj);
            if (note->note_type == NoteType::HOLD) {
                Hold* hold = std::static_pointer_cast<Hold>(note).get();
                ui->timestam_value->setText(
                    QString::number(hold->timestamp, 'f', 0) + "\n" +
                    tr("Hold time:") +
                    QString::number(hold->hold_time, 'f', 0) + "ms");
            } else {
                // 其他
                ui->timestam_value->setText(
                    QString::number(current_obj->timestamp, 'f', 0));
            }
            // 更新轨道信息
            ui->orbit_value->setText(QString::number(note->orbit + 1));
        }
        // 更新分拍位置信息
        if (current_beatinfo) {
            ui->divisor_value->setText(
                QString::number(current_obj->divpos) + "/" +
                QString::number(current_beatinfo->divisors));
        } else {
            ui->divisor_value->setText("0/1");
        }
    }
    if (current_ref_timing) {
        // 更新timing标签
        ui->ntiming_time_value->setText(
            QString::number(current_ref_timing->timestamp, 'f', 0));
        ui->ntiming_bpm_value->setText(
            QString::number(current_ref_timing->basebpm, 'f', 2));
    }

    // 无选中时隐藏组件
    if (current_obj == nullptr && current_beatinfo == nullptr &&
        current_ref_timing == nullptr) {
        if (!isHidden()) {
            hide();
        }
    } else {
        if (isHidden()) {
            setHidden(false);
        }
    }
}
