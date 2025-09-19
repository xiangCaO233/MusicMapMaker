#ifndef TIMINGMANAGER_H
#define TIMINGMANAGER_H

#include <qvalidator.h>

#include <QWidget>
#include <tool/ThreadSafeQueue.hpp>
#include <tool/command/ToolCommand.hpp>

namespace Ui {
class TimingEditor;
}

class MMap;
class MProject;
class Timing;

class TimingManager : public QWidget {
    Q_OBJECT
    static QStringList timing_metaNames();

   public:
    explicit TimingManager(QWidget *parent = nullptr);
    ~TimingManager();

   signals:
    void navigateToTiming(Timing *timing);
    void updateTiming(Timing *timing, std::unique_ptr<Timing> &desdata);

   public slots:
    void onMapUpdated(MProject *project, MMap *map);
    void bind_toolcmdq(ThreadSafeQueue<ToolCommand> *tool_cmd_queue);

   private:
    Ui::TimingEditor *ui;
    MMap *map_ref;
    ThreadSafeQueue<ToolCommand> *tool_cmd_queue;
    QIntValidator *timeInputValidator;
    QDoubleValidator *parameterInputValidator;
};

#endif  // TIMINGMANAGER_H
