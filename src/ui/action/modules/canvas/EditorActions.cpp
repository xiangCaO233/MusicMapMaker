#include <qkeysequence.h>

#include <QKeySequence>
#include <action/ActionManager.hpp>
#include <action/modules/canvas/EditorActions.hpp>

void EditorActions::createActions() {
    auto action_manager = ActionManager::instance();

    action_manager->createAction("canvas.pause_or_resume", tr("Pause/Resume"),
                                 QKeySequence::fromString("Space"));
    action_manager->createAction("canvas.cancel", tr("Cancel"),
                                 QKeySequence::Cancel);

    action_manager->createAction("canvas.selectpage", tr("Select Page"),
                                 QKeySequence::SelectAll);
    action_manager->createAction("canvas.selectall", tr("Select All"),
                                 QKeySequence::fromString("Ctrl+Shift+A"));

    action_manager->createAction("canvas.cut", tr("Cut"), QKeySequence::Cut);
    action_manager->createAction("canvas.copy", tr("Copy"), QKeySequence::Copy);
    action_manager->createAction("canvas.paste", tr("Paste"),
                                 QKeySequence::Paste);
    action_manager->createAction("canvas.delete", tr("Delete"),
                                 QKeySequence::Delete);

    action_manager->createAction("canvas.undo", tr("Undo"), QKeySequence::Undo);
    action_manager->createAction("canvas.redo", tr("Redo"), QKeySequence::Redo);
    action_manager->createAction("canvas.find", tr("Find"), QKeySequence::Find);
}
