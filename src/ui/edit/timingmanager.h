#ifndef TIMINGMANAGER_H
#define TIMINGMANAGER_H

#include <QStandardItem>
#include <QWidget>

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
    void updateTiming(Timing *timing);

   public slots:
    void onTimingItemChanged(QStandardItem *item);
    void onMapUpdated(MProject *project, MMap *map);

   private:
    Ui::TimingEditor *ui;
    MMap *map_ref;
};

#endif  // TIMINGMANAGER_H
