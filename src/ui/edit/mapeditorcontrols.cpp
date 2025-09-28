#include <mapeditor.h>
#include <ui_mapeditor.h>

#include <info/MapCanvasInfo.hpp>
#include <util/mutil.hpp>

void MapEditor::on_timeline_effect_button_toggled(bool checked) {
    // 切换时间线映射效果
    auto info = canvas()->info<MapCanvasInfo>();
    info->baseInfo.timeline_mapping_type =
        checked ? TimeLineMappingType::EFFECTED : TimeLineMappingType::LINEAR;
}

void MapEditor::on_magnet_to_divisor_button_toggled(bool checked) {
    // 切换吸附到分拍线
    auto info = canvas()->info<MapCanvasInfo>();
    info->editorInfo.magnet_to_divisor = checked;
}

void MapEditor::on_scroll_direction_button_toggled(bool checked) {
    auto info = canvas()->info<MapCanvasInfo>();
    info->editorInfo.scrollInfo.scroll_natural = checked;

    QColor color;
    switch (current_theme) {
        case GlobalTheme::OPEN_DARK:
        case GlobalTheme::COLIN_DARK: {
            color = Qt::white;
            break;
        }
        case GlobalTheme::OPEN_LIGHT:
        case GlobalTheme::COLIN_LIGHT: {
            color = Qt::black;
            break;
        }
    }

    auto iconres = checked ? "://icons/long-arrow-alt-down.svg"
                           : "://icons/long-arrow-alt-up.svg";
    // 切换图标
    mutil::set_button_svgcolor(ui->scroll_direction_button, iconres, color, 16,
                               16);
}
