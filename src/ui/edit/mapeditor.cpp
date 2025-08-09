#include "mapeditor.h"

#include "ui_mapeditor.h"

MapEditor::MapEditor(QWidget* parent)
    : HideableToolWindow(parent), ui(new Ui::MapEditor) {
    ui->setupUi(this);
    auto canvas = ui->canvas_container->canvas.data();
    connect(canvas, &GLCanvas::update_window_suffix, this,
            &MapEditor::update_title_suffix);
}

MapEditor::~MapEditor() { delete ui; }

MapCanvas* MapEditor::canvas() const {
    return ui->canvas_container->canvas.data();
}

// 更新标题后缀
void MapEditor::update_title_suffix(const QString& suffix) {
    setWindowTitle(tr("Editor-->") + suffix);
}
