#ifndef TIMINGMANAGER_H
#define TIMINGMANAGER_H

#include <qvalidator.h>

#include <QWidget>
#include <TimingTableUsefulWidgets.hpp>
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
    MMap *map_ref{nullptr};
    ThreadSafeQueue<ToolCommand> *tool_cmd_queue;
    QIntValidator *timeInputValidator;
    QDoubleValidator *parameterInputValidator;
    // 这个列表是核心！它按视觉顺序存储了每一行的控制器。
    QList<TimingRowItem *> allTimingRowItems;

    // 用于持有“添加”按钮行的特殊指针
    AddTimingItem *addTimingItem{nullptr};

    // 添加一个新timing行
    void addNewTimingRowItem(MMap *mapref, Timing *newTiming);

    // 从map刷新timing表
    void refreshTableFromMap();

    // 声明一个排序和重建表格的函数
    void sortAndRebuildTable();
};

#endif  // TIMINGMANAGER_H
