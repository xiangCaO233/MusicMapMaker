#ifndef M_CANVASSETTINGS_H
#define M_CANVASSETTINGS_H

struct CanvasSettings {
    // 判定线位置:从下往上此倍率*总高度
    double judgeline_position{0.16};

    // 信息区宽度倍率:实际宽度为总宽度*preview_width_scale
    double infoarea_width_scale{0.24};

    // 预览区宽度倍率:实际宽度为总宽度*preview_width_scale
    double preview_width_scale{0.22};

    // 判定线后是否显示物件
    bool show_object_after_judgeline{true};

    // 选中框边框宽度(pix)
    double select_border_width{6};

    // 是否严格选中
    bool strict_select{false};

    // 是否显示时间线
    bool show_timeline{true};
};

#endif  // M_CANVASSETTINGS_H
