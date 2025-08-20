#ifndef MMM_FILEACTIONHANDLER_HPP
#define MMM_FILEACTIONHANDLER_HPP

#include <QObject>

class FileActionHandler : public QObject {
    Q_OBJECT

   public:
    // 构造FileActionHandler
    explicit FileActionHandler(QObject* parent = nullptr);

    // 析构FileActionHandler
    ~FileActionHandler() override;

   public slots:
    void onNewProject();
    void onNewFile();
    void onOpen();

    void onSave();
    void onSaveAs();
    void onExport();
};

#endif  // MMM_FILEACTIONHANDLER_HPP
