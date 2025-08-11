#include <qobject.h>

#include <QCoreApplication>
#include <QEvent>
#include <layer/LayerManager.hpp>
#include <render/synchronize/FrameSynchronizer.hpp>
#include <render/synchronize/RenderDataLoop.hpp>

// 构造RenderTick
RenderDataLoop::RenderDataLoop(QObject *parent) : QObject(parent) {
    // 初始化图层管理器
    layer_manager = std::make_unique<LayerManager>();
}

// 析构RenderTick
RenderDataLoop::~RenderDataLoop() = default;

// 启动循环
void RenderDataLoop::start() {
    if (isRunning) return;
    isRunning = true;
    timer.restart();

    // 立即开始第一次tick，后续tick会自我驱动
    QCoreApplication::postEvent(this, new QEvent(QEvent::User));
}

// 停止循环
void RenderDataLoop::stop() { isRunning = false; }

// 设置目标fps
// (最大1000)
void RenderDataLoop::set_targetFPS(qreal fps) {
    if (fps > 1000) {
        desiredTicktimeNs = uint64_t(std::floor(1000000000. / fps / 2.));
    } else {
        desiredTicktimeNs = 1000000 / 2;
    }
    qDebug() << "destickTime:" << desiredTicktimeNs;
}

// 一个数据刻
void RenderDataLoop::tick() {
    actualTicktimeNs = timer.nsecsElapsed();
    timer.restart();

    // 开始新一帧：打开栅栏A，让所有图层线程开始计算
    layer_manager->sync().startNextFrame();

    // ... 在此期间主tick线程也可以做其他事情

    // 等待所有计算完成：在栅栏B处阻塞等待
    layer_manager->sync().waitForAllWorkers();

    // 交换缓冲区
    layer_manager->swapBuffers();

    emit renderUpdate();

    // 帧率限制逻辑
    qint64 workTimeNs = timer.nsecsElapsed();
    if (qint64 sleepTimeNs = desiredTicktimeNs - workTimeNs; sleepTimeNs > 0) {
        // 使用精确的休眠
        // qDebug() << "fix time:" << sleepTimeNs;
        QThread::msleep(sleepTimeNs / 1000000);
        // QCoreApplication::processEvents(QEventLoop::AllEvents,
        //                                 sleepTimeNs / 1000000);
        actualTicktimeNs = sleepTimeNs + workTimeNs;
    } else {
        actualTicktimeNs = workTimeNs;
    }

    // 安排下一次tick,使用postEvent可以避免栈溢出,并允许Qt处理其他事件
    QCoreApplication::postEvent(this, new QEvent(QEvent::User));
}

// 还需要重写 event() 函数来处理我们自己post的事件
bool RenderDataLoop::event(QEvent *e) {
    if (e->type() == QEvent::User) {
        tick();
        return true;
    }
    return QObject::event(e);
}
