#ifndef M_FRAMERATECOUNTER_H
#define M_FRAMERATECOUNTER_H

#include <QObject>
#include <QTimer>
#include <QWidget>

// 帧率计数器
class FrameRateCounter : public QObject {
    Q_OBJECT
   public:
    FrameRateCounter(QWidget *parent = nullptr);
    void frameRendered();

   signals:
    void fpsUpdated(int fps);

   private:
    void updateFPS();

    int frameCount = 0;
    QTimer timer;
};

#endif  // M_FRAMERATECOUNTER_H
