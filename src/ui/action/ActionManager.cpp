#include <action/ActionManager.hpp>
#include <action/modules/file/FileActions.hpp>

ActionManager::ActionManager(QObject* parent) : QObject(parent) {}

// 注册事件
void ActionManager::createAction(const QString& id, const QString& text,
                                 const QKeySequence& shortcut) {
    if (!actions.contains(id)) {
        auto action = new QAction(text, this);
        action->setShortcut(shortcut);
        actions.insert(id, action);
    }
}
// 注册事件
void ActionManager::createAction(const QString& id, const QString& text,
                                 const QList<QKeySequence>& shortcuts) {
    if (!actions.contains(id)) {
        auto action = new QAction(text, this);
        action->setShortcuts(shortcuts);
        actions.insert(id, action);
    }
}

QAction* ActionManager::getAction(const QString& id) {
    return actions.value(id, nullptr);
}

void ActionManager::connectCommand(const QString& actionId,
                                   const QObject* receiver, const char* slot) {
    auto action = getAction(actionId);
    if (action) {
        connect(action, SIGNAL(triggered()), receiver, slot);
    }
}
