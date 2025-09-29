#include <mapeditor.h>
#include <ui_mapeditor.h>

#include <util/mutil.hpp>

MapEditor::MapEditor(QWidget* parent) : QWidget(parent), ui(new Ui::MapEditor) {
    ui->setupUi(this);
    initializeMenus();
}

MapEditor::~MapEditor() { delete ui; }

// 使用主题
void MapEditor::use_theme(GlobalTheme theme) {
    current_theme = theme;
    QColor color;
    switch (theme) {
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
    mutil::set_toolbutton_svgcolor(ui->edit_toolsbutton,
                                   "://icons/hand-rock.svg", color, 16, 16);
    mutil::set_toolbutton_svgcolor(ui->generate_divisors_toolbutton,
                                   "://icons/lines.svg", color, 16, 16);
    mutil::set_toolbutton_svgcolor(ui->bg_adjust_toolbutton,
                                   "://icons/background.svg", color, 16, 16);

    // 按钮组
    // mutil::set_toolbutton_svgcolor(hand_mode_button,
    // "://icons/hand-rock.svg",
    //                                color, 16, 16);
    // mutil::set_toolbutton_svgcolor(note_mode_button, "://icons/edit.svg",
    // color,
    //                                16, 16);

    // 切换按钮
    mutil::set_button_svgcolor(ui->timeline_effect_button,
                               "://icons/effect.svg", color, 16, 16);
    mutil::set_button_svgcolor(ui->magnet_to_divisor_button,
                               "://icons/magnet.svg", color, 16, 16);
    mutil::set_button_svgcolor(ui->scroll_direction_button,
                               "://icons/long-arrow-alt-up.svg", color, 16, 16);
    canvas()->use_theme(theme);
}

MapCanvas* MapEditor::canvas() const {
    return ui->canvas_container->canvas.data();
}
