#ifndef MMM_FILEACTIONHANDLER_HPP
#define MMM_FILEACTIONHANDLER_HPP

#include <QObject>

class FileActionHandler : public QObject {
    Q_OBJECT

   public:
    inline static FileActionHandler* instance() {
        static FileActionHandler instance;
        return &instance;
    }
    // 构造FileActionHandler
    explicit FileActionHandler(QObject* parent = nullptr);

    // 析构FileActionHandler
    ~FileActionHandler() override;

   signals:
    void open(std::string file);
    void open_directory(std::string dir);
    void save();

    // 需要默认名称
    void save_as();
    void export_as();

   public slots:
    void onNewProject();
    void onNewFile();
    void onOpen();
    void onOpenDirectory();

    void onSave();
    void onSaveAs();
    void onExport();
};

#endif  // MMM_FILEACTIONHANDLER_HPP
