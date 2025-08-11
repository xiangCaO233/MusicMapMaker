#include <render/synchronize/FrameSynchronizer.hpp>

FrameSynchronizer::FrameSynchronizer(int worker_count, QObject* parent)
    : QObject(parent), workerCount(worker_count) {}

void FrameSynchronizer::startNextFrame() {
    // 释放N个许可，允许N个worker通过“开始”栅栏
    startGate.release(workerCount);
}

void FrameSynchronizer::waitForAllWorkers() {
    // 尝试获取N个许可，如果不够，就会阻塞，直到所有worker都调用了workerFinishedFrame
    finishGate.acquire();
};

void FrameSynchronizer::workerWaitForFrameStart() {
    // 尝试获取一个“开始”许可，如果主循环还没调用startNextFrame，就会阻塞
    startGate.acquire();
};

// workerFinishedFrame 再修正
void FrameSynchronizer::workerFinishedFrame() {
    QMutexLocker locker(&mutex);
    waitCount++;
    if (waitCount == workerCount) {
        // 是最后一个，唤醒所有其他等待的worker
        // 进入下一帧周期
        waitCount = 0;
        // 通知主循环可以继续了
        finishGate.release();
        finishCondition.wakeAll();
    } else {
        // 不是最后一个，在此等待，直到被唤醒
        finishCondition.wait(&mutex);
    }
}
