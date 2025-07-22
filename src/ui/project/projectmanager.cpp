#include "projectmanager.h"

#include <qlogging.h>

#include <QCloseEvent>

#include "ui_projectmanager.h"

ProjectManager::ProjectManager(QWidget* parent)
    : QWidget(parent), ui(new Ui::ProjectManager) {
    ui->setupUi(this);
    ui->main_splitter->setSizes({220, 800});
    ui->main_splitter->setSizes({300, 420});
    ui->project_content_splitter->setSizes({420, 300});
}

ProjectManager::~ProjectManager() {
    delete ui;
    qDebug() << "ProjectManager deleted";
}

// 把关闭事件改为hide
void ProjectManager::closeEvent(QCloseEvent* event) {
    hide();
    emit close_signal();
    event->ignore();
}
