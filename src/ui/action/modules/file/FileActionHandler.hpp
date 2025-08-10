#ifndef MMM_FILEACTIONHANDLER_HPP
#define MMM_FILEACTIONHANDLER_HPP

#include <qobject.h>

class FileActionHandler : public QObject {
    Q_OBJECT

   public:
    // 构造FileActionHandler
    explicit FileActionHandler(QObject* parent);

    // 析构FileActionHandler
    ~FileActionHandler() override;
};

#endif  // MMM_FILEACTIONHANDLER_HPP
