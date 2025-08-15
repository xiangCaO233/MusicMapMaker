#include <mapeditor.h>
#include <ui_mapeditor.h>

MapEditor::MapEditor(QWidget* parent) : QWidget(parent), ui(new Ui::MapEditor) {
    ui->setupUi(this);
    initializeMenus();
}

MapEditor::~MapEditor() { delete ui; }

MapCanvas* MapEditor::canvas() const {
    return ui->canvas_container->canvas.data();
}
