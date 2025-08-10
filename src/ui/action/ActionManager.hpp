#ifndef MMM_ACTIONMANAGER_HPP
#define MMM_ACTIONMANAGER_HPP

#include <QAction>
#include <QKeySequence>
#include <QObject>
#include <QString>

// 单例管理器
class ActionManager : public QObject {
    Q_OBJECT
   public:
    inline static ActionManager* instance() {
        static ActionManager instance;
        return &instance;
    }

    // 注册事件
    void createAction(const QString& id, const QString& text,
                      const QKeySequence& shortcut = QKeySequence());
    QAction* getAction(const QString& id);
    void connectCommand(const QString& actionId, const QObject* receiver,
                        const char* slot);

   private:
    explicit ActionManager(QObject* parent = nullptr);
    ActionManager(const ActionManager&) = delete;
    ActionManager& operator=(const ActionManager&) = delete;

    QHash<QString, QAction*> actions;
};

#endif  // MMM_ACTIONMANAGER_HPP
