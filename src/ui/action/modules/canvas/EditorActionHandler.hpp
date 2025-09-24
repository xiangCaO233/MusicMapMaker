#ifndef MMM_EDITORACTIONHANDLER_HPP
#define MMM_EDITORACTIONHANDLER_HPP

#include <QObject>

class EditorActionHandler : public QObject {
    Q_OBJECT
   public:
    inline static EditorActionHandler* instance() {
        static EditorActionHandler instance;
        return &instance;
    }
    // 构造EditorActionHandler
    explicit EditorActionHandler(QObject* parent = nullptr);
    // 析构EditorActionHandler
    ~EditorActionHandler() override;

   signals:
    void pause_or_resume_canvas();
    void cut();
    void copy();
    void paste();
    void delete_signal();
    void undo();
    void redo();

   public slots:
    void onPause_Resume();
    void onCancel();

    void onSelectPage();
    void onSelectAll();

    void onCut();
    void onCopy();
    void onPaste();
    void onDelete();

    void onUndo();
    void onRedo();

    void onFind();
};
#endif  // MMM_EDITORACTIONHANDLER_HPP
