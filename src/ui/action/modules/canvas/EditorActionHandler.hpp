#ifndef MMM_EDITORACTIONHANDLER_HPP
#define MMM_EDITORACTIONHANDLER_HPP

#include <QObject>

class EditorActionHandler : public QObject {
    Q_OBJECT
   public:
    // 构造EditorActionHandler
    explicit EditorActionHandler(QObject* parent = nullptr);
    // 析构EditorActionHandler
    ~EditorActionHandler() override;

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
