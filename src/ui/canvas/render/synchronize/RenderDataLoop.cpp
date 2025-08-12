#include <QCoreApplication>
#include <QEvent>
#include <QObject>
#include <algorithm>
#include <layer/LayerManager.hpp>
#include <render/synchronize/FrameSynchronizer.hpp>
#include <render/synchronize/RenderDataLoop.hpp>

// 构造RenderTick
RenderDataLoop::RenderDataLoop(Renderer2D *renderer, QObject *parent)
    : QObject(parent) {
    // 初始化图层管理器
    layer_manager = std::make_unique<LayerManager>(renderer);
}

// 析构RenderTick
RenderDataLoop::~RenderDataLoop() = default;

// 启动循环
void RenderDataLoop::start() {
    if (isRunning) return;
    isRunning = true;
    timer.restart();

    // 立即开始第一次tick
    // 后续tick自我驱动
    QCoreApplication::postEvent(this, new QEvent(QEvent::User));
}

// 停止循环
void RenderDataLoop::stop() { isRunning = false; }

// 1s接收一个
void RenderDataLoop::updateFPS(int fps) {
    if (desiredFps <= 0) {
        sleepAdjustmentNs.store(0);
        return;
    }

    // 1. 更新滑动窗口平均值
    fps_history.enqueue(fps);
    while (fps_history.size() > HISTORY_SECONDS) {
        fps_history.dequeue();
    }

    double avg_fps = 0.0;
    if (!fps_history.isEmpty()) {
        double sum = 0;
        for (int val : fps_history) {
            sum += val;
        }
        avg_fps = sum / fps_history.size();
    } else {
        avg_fps = fps;  // 如果历史为空，直接使用当前值
    }

    // 2. 计算当前误差 (单位: ns)
    qint64 desiredFrameTimeNs = 1000000000.0 / desiredFps;
    qint64 actualFrameTimeNs =
        (avg_fps > 0) ? (1000000000.0 / avg_fps) : desiredFrameTimeNs;

    auto errorNs = static_cast<double>(desiredFrameTimeNs - actualFrameTimeNs);

    // 3. --- PID 控制器 ---
    // 这些增益值是一个更保守、更稳定的起点，需要根据实际效果微调
    const qreal P_GAIN = 0.12;  // 从 0.08 -> 0.12
    const qreal I_GAIN = 0.05;  // 从 0.02 -> 0.05 (提高2.5倍)
    const qreal D_GAIN = 0.06;  // 略微增大D以应对可能增加的震荡

    // P (比例) 项: 响应当前误差
    double proportional_term = errorNs * P_GAIN;

    // I (积分) 项: 消除稳态误差
    integral_error_ns += errorNs * I_GAIN;

    // 积分抗饱和 (Windup Guard): 防止积分项无限累积
    // 将累积上限设为目标帧时间的一半，这是一个经验值
    double max_integral = desiredFrameTimeNs / 2.0;
    integral_error_ns =
        std::clamp(integral_error_ns, -max_integral, max_integral);

    // D (微分) 项: 抑制震荡
    double derivative_term = (errorNs - last_error_ns) * D_GAIN;

    // 更新上一次的误差，为下一次计算做准备
    last_error_ns = errorNs;

    // 总调整量 = P项 + I项 + D项
    auto adjustment = static_cast<int64_t>(proportional_term +
                                           integral_error_ns + derivative_term);

    sleepAdjustmentNs.store(adjustment);

    // qDebug() << "PID Control: desired=" << QString::number(desiredFps, 'f',
    // 1)
    //          << ", avg=" << QString::number(avg_fps, 'f', 1)
    //          << ", error=" << QString::number(errorNs / 1000.0, 'f', 1) <<
    //          "us"
    //          << ", P=" << QString::number(proportional_term / 1000.0, 'f', 1)
    //          << "us"
    //          << ", I=" << QString::number(integral_error_ns / 1000.0, 'f', 1)
    //          << "us"
    //          << ", D=" << QString::number(derivative_term / 1000.0, 'f', 1)
    //          << "us"
    //          << ", total_adj=" << QString::number(adjustment / 1000.0, 'f',
    //          1)
    //          << "us";
}

// 设置目标fps
// (实际最大500(qt事件循环消耗至少2ms))
void RenderDataLoop::set_targetFPS(qreal fps) {
    if (fps < 500) {
        if (fps == 0) {
            desiredTicktimeNs = 0;
            desiredFps = 500;
        } else {
            desiredTicktimeNs = uint64_t(std::floor(1000000000. / fps / 2.));
            desiredFps = fps;
        }
    } else {
        desiredTicktimeNs = 2000000 / 2;
        desiredFps = 500;
    }
    qDebug() << "destickTime:" << desiredTicktimeNs;
}

// 一个数据刻
void RenderDataLoop::tick() {
    actualTicktimeNs = timer.nsecsElapsed();
    timer.restart();

    // 开始新一帧:打开栅栏A，让所有图层线程开始计算
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
        QThread::usleep((sleepTimeNs + sleepAdjustmentNs.load()) / 1000);
        // QCoreApplication::processEvents(QEventLoop::AllEvents,
        //                                 sleepTimeNs / 1000000);
        actualTicktimeNs = sleepTimeNs + workTimeNs;
    } else {
        actualTicktimeNs = workTimeNs;
    }

    // 安排下一次tick,使用postEvent可以避免栈溢出,并允许Qt处理其他事件
    QCoreApplication::postEvent(this, new QEvent(QEvent::User));
}

bool RenderDataLoop::event(QEvent *e) {
    if (e->type() == QEvent::User) {
        tick();
        return true;
    }
    return QObject::event(e);
}
