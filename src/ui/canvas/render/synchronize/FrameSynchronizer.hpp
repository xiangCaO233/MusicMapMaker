#ifndef MMM_FRAMESYNCHRONIZER_HPP
#define MMM_FRAMESYNCHRONIZER_HPP

#include <qobject.h>

#include <QMutex>
#include <QSemaphore>
#include <QWaitCondition>

class FrameSynchronizer : public QObject {
    Q_OBJECT
   public:
    explicit FrameSynchronizer(int workerCount, QObject* parent = nullptr);
    ~FrameSynchronizer() override = default;

    // --- 由主循环调用 ---
    void startNextFrame();     // 打开栅栏A，让所有worker开始工作
    void waitForAllWorkers();  // 在栅栏B等待所有worker完成

    // --- 由工作线程调用 ---
    void workerWaitForFrameStart();  // Worker在栅栏A等待
    void workerFinishedFrame();      // Worker在栅栏B报告完成

   private:
    int workerCount;
    QMutex mutex;

    // 用于 开始计算 栅栏 (栅栏A)
    QSemaphore startGate{0};

    QWaitCondition finishCondition;
    // 在栅栏处等待的线程数
    int waitCount{0};

    // 用于 计算完成 栅栏 (栅栏B)
    QSemaphore finishGate{0};
    int finishedCount{0};
};

#endif  // MMM_FRAMESYNCHRONIZER_HPP
