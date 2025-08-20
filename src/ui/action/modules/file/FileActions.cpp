#include <QKeySequence>
#include <action/ActionManager.hpp>
#include <action/modules/file/FileActions.hpp>

void FileActions::createActions() {
    auto action_manager = ActionManager::instance();

    action_manager->createAction(
        "project.new", tr("New Project..."),
        QKeySequence::AddTab);  // ctrl + t/ctrl + shift + n
    action_manager->createAction("file.new", tr("New File...(&N)"),
                                 QKeySequence::New);  // ctrl + n
    action_manager->createAction("file.open", tr("Open File/Project...(&O)"),
                                 QKeySequence::Open);  // ctrl + n

    action_manager->createAction("file.save", tr("Save(&S)"),
                                 QKeySequence::Save);  // ctrl + s
    action_manager->createAction("file.saveas", tr("Save As..."),
                                 QKeySequence::SaveAs);  // ctrl + shift + s
    action_manager->createAction(
        "file.export", tr("Export..."),
        QKeySequence::fromString(
            "Ctrl+Alt+Shift+S"));  // ctrl + alt + shift + s
}
