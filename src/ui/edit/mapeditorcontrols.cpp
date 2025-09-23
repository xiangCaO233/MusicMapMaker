#include <mapeditor.h>
#include <ui_mapeditor.h>

#include <info/MapCanvasInfo.hpp>

#include "util/mutil.hpp"

void MapEditor::on_timeline_effect_button_toggled(bool checked) {
    // 切换时间线映射效果
    auto info = canvas()->info<MapCanvasInfo>();
    info->baseInfo.timeline_mapping_type =
        checked ? TimeLineMappingType::EFFECTED : TimeLineMappingType::LINEAR;
}

void MapEditor::on_magnet_to_divisor_button_toggled(bool checked) {
    // 切换吸附到分拍线
    auto info = canvas()->info<MapCanvasInfo>();
    info->baseInfo.magnet_to_divisor = checked;
}

void MapEditor::on_scroll_direction_button_toggled(bool checked) {
    auto info = canvas()->info<MapCanvasInfo>();
    info->baseInfo.scroll_natural = checked;

    QColor color = Qt::white;
    auto iconres = checked ? "://icons/long-arrow-alt-down.svg"
                           : "://icons/long-arrow-alt-up.svg";
    // 切换图标
    mutil::set_button_svgcolor(ui->scroll_direction_button, iconres, color, 16,
                               16);
}
