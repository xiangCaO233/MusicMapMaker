#include <QCloseEvent>
#include <template/HideableToolWindow.hpp>

HideableToolWindow::HideableToolWindow(QWidget* parent) : QWidget(parent) {}

HideableToolWindow::~HideableToolWindow() = default;

// 把关闭事件改为hide
void HideableToolWindow::closeEvent(QCloseEvent* event) {
    hide();
    emit close_signal();
    event->ignore();
}
