#include <QKeySequence>
#include <action/ActionManager.hpp>
#include <action/modules/file/FileActions.hpp>

void FileActions::createActions() {
    auto action_manager = ActionManager::instance();
    action_manager->createAction("file.new", "新建", QKeySequence::New);
}
