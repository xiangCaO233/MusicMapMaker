#ifndef MMM_EDITORACTIONS_HPP
#define MMM_EDITORACTIONS_HPP

#include <QObject>

// 创建所有快捷键
class EditorActions : public QObject {
    Q_OBJECT
   public:
    using QObject::QObject;
    ~EditorActions() override = default;

    static void createActions();
};

#endif  // MMM_EDITORACTIONS_HPP
