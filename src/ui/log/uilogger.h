#ifndef M_UILOGGER_H
#define M_UILOGGER_H

#include <qobject.h>
#include <qtmetamacros.h>

class MUiLogger : public QObject {
    Q_OBJECT
   public:
    // 构造MUiLogger
    MUiLogger() {}

    // 析构MUiLogger
    virtual ~MUiLogger() = default;

   signals:
    void info(const QString& message);
    void warn(const QString& message);
    void error(const QString& message);
};

#endif  // M_UILOGGER_H
