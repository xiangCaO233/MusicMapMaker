#include <qkeysequence.h>

#include <QKeySequence>
#include <action/ActionManager.hpp>
#include <action/modules/canvas/EditorActions.hpp>

void EditorActions::createActions() {
    auto action_manager = ActionManager::instance();
    action_manager->createAction("canvas.cancel", "取消", QKeySequence::Cancel);
    action_manager->createAction("canvas.pause_or_resume", "暂停/继续",
                                 QKeySequence::fromString("Space"));
}
