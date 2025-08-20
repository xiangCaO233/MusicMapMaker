#ifndef MMM_FILEACTIONS_HPP
#define MMM_FILEACTIONS_HPP

#include <QObject>

// 创建所有快捷键
class FileActions : public QObject {
    Q_OBJECT
   public:
    using QObject::QObject;
    ~FileActions() override = default;

    static void createActions();
};

#endif  // MMM_FILEACTIONS_HPP
