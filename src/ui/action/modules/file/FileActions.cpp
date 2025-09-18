#include <QKeySequence>
#include <action/ActionManager.hpp>
#include <action/modules/file/FileActions.hpp>

void FileActions::createActions() {
    auto action_manager = ActionManager::instance();

    action_manager->createAction(
        "project.new", tr("New Project..."),
        QKeySequence::AddTab);  // ctrl + t/ctrl + shift + n
    action_manager->createAction("file.new", tr("New File...(&n)"),
                                 QKeySequence::New);  // ctrl + n
    action_manager->createAction("file.open", tr("Open File...(&o)"),
                                 QKeySequence::Open);  // ctrl + o
    action_manager->createAction(
        "file.openDirectory", tr("Open Directory...(&O)"),
        QKeySequence("Ctrl+Shift+O"));  // ctrl + shift + o

    action_manager->createAction("file.save", tr("Save(&s)"),
                                 QKeySequence::Save);  // ctrl + s
    action_manager->createAction("file.saveas", tr("Save As..."),
                                 QKeySequence::SaveAs);  // ctrl + shift + s
    action_manager->createAction(
        "file.export", tr("Export..."),
        QKeySequence::fromString(
            "Ctrl+Alt+Shift+S"));  // ctrl + alt + shift + s
}
