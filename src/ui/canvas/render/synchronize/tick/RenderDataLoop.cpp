#include <QCoreApplication>
#include <QEvent>
#include <QObject>
#include <QThread>
#include <algorithm>
#include <layer/LayerManager.hpp>
#include <render/synchronize/FrameSynchronizer.hpp>
#include <render/synchronize/tick/RenderDataLoop.hpp>
#include <render/synchronize/tick/RenderLoopWorker.hpp>

// 构造RenderTick

RenderDataLoop::RenderDataLoop(Renderer2D *renderer, QObject *parent)
    : QObject(parent), render(renderer) {
    // 构造函数：创建但不启动线程
    m_worker_thread = new QThread(this);
    m_worker = new RenderLoopWorker(this);

    // 将工作者移动到新线程
    m_worker->moveToThread(m_worker_thread);

    // 连接，当线程启动时，开始执行 doWork 循环
    connect(m_worker_thread, &QThread::started, m_worker,
            &RenderLoopWorker::doWork);

    // （可选）当循环结束时，可以自动清理 worker
    // connect(m_worker_thread, &QThread::finished, m_worker,
    // &QObject::deleteLater);
}

// 析构RenderTick
RenderDataLoop::~RenderDataLoop() {
    stop();  // 确保析构时能正确停止
}

void RenderDataLoop::start() {
    if (isRunning.load()) return;
    isRunning.store(true);
    // 启动后台线程，这会自动触发 doWork 的调用
    m_worker_thread->start();
}

void RenderDataLoop::stop() {
    if (!isRunning.load()) return;
    isRunning.store(false);

    // 等待线程优雅退出
    m_worker_thread->quit();
    if (!m_worker_thread->wait(5000)) {
        qWarning() << "RenderDataLoop thread did not stop gracefully, forcing "
                      "termination.";
        m_worker_thread->terminate();
        m_worker_thread->wait();
    }
}

void RenderDataLoop::set_targetFPS(qreal fps) {
    desired_fps = (fps > 0) ? fps : 0.0;
    if (desired_fps > 0) {
        desired_frame_time_ns = 1000000000.0 / desired_fps;
    } else {
        desired_frame_time_ns = 0;
    }
}

// updateFPS 槽函数现在需要线程安全地更新数据
// 但实际上，它根本不需要了！因为我们不再有PID控制器
// 我们可以保留这个接口，但让它什么都不做，或者只用于显示
void RenderDataLoop::updateFPS(int fps) {
    // 这个函数现在是可选的，因为节拍器是开环控制
    // 我们可以用它来记录和显示实际帧率，但它不再参与控制
    // qDebug() << "Actual measured FPS:" << fps;
}

// tick() 和 event() 函数现在是空的！
// 因为所有逻辑都移动到了 RenderLoopWorker::doWork 中
void RenderDataLoop::tick() {
    // This function is now intentionally empty.
}

// 可重写的tick事件(执行其他任务)
void RenderDataLoop::tickEvent() {}
void RenderDataLoop::pre_tickEvent() {}
void RenderDataLoop::after_tickEvent() {}

// 初始化层管理器
void RenderDataLoop::initializeLayerManager() {
    // 初始化图层管理器
    layer_manager = std::make_unique<LayerManager>(render);
    layer_manager->initializeLayers();
}

void RenderDataLoop::updateMap(MMap *map) { layer_manager->updateMap(map); }

// 更新信息
void RenderDataLoop::update_info(SharedCanvasInfo *newinfo) {
    info = newinfo;
    layermanager()->updateInfoForLayers(newinfo);
}
