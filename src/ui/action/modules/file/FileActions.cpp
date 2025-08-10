#include <qkeysequence.h>

#include <action/modules/file/FileActions.hpp>

#include "action/ActionManager.hpp"

void FileActions::createActions() {
    auto action_manager = ActionManager::instance();
    action_manager->createAction("file.new", "新建", QKeySequence::New);
}
