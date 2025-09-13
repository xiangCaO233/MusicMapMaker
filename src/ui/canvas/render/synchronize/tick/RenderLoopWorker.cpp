#include <QThread>
#include <info/MapCanvasInfo.hpp>
#include <layer/LayerManager.hpp>
#include <render/synchronize/tick/RenderDataLoop.hpp>
#include <render/synchronize/tick/RenderLoopWorker.hpp>

// --- 私有工作者的实现 ---
RenderLoopWorker::RenderLoopWorker(RenderDataLoop* outer) : d(outer) {}

void RenderLoopWorker::doWork() {
    d->timer.start();
    d->next_tick_time_ns = d->timer.nsecsElapsed();

    // 定义一个“切换到忙等待”的时间阈值 (单位: 纳秒)
    // 这是一个关键的可调参数。500微秒 (500,000 ns) 是一个合理的起点。
    const qint64 busy_wait_threshold_ns = 20000;

    while (d->isRunning.load()) {
        // --- 1. 智能混合等待 ---
        qint64 current_time_ns = d->timer.nsecsElapsed();
        if (qint64 time_to_wait_ns = d->next_tick_time_ns - current_time_ns;
            time_to_wait_ns > 0) {
            // 如果需要等待的时间大于我们的阈值
            if (time_to_wait_ns > busy_wait_threshold_ns) {
                // 进行一次性的睡眠，留出阈值的时间用于后续的忙等待
                QThread::usleep((time_to_wait_ns - busy_wait_threshold_ns) /
                                1000);
            }

            // --- 精确的忙等待阶段 ---
            // 无论是从睡眠中唤醒，还是因为等待时间本来就短
            // 在这里进行最终的、精确的等待
            while (d->timer.nsecsElapsed() < d->next_tick_time_ns) {
                std::this_thread::yield();
                // 在现代CPU上，这个指令可以减少CPU在紧密循环中的功耗
                // 对于Intel是 _mm_pause()，对于ARM是 __yield()
                // C++20 提供了 std::this_thread::yield()，但它作用不同
                // 在这里，一个空循环通常已经足够了，或者使用特定于平台的指令
                // #if defined(__i386__) || defined(__x86_64__)
                //                 _mm_pause();  // 需要 #include <immintrin.h>
                // #endif
            }
        }

        // --- 2. 计算时间信息 ---
        auto raw_delta_ms = double(d->desired_frame_time_ns / 1000.0) / 1000.0;

        // --- 3. 计算下一拍目标 ---
        d->next_tick_time_ns += d->desired_frame_time_ns;

        // --- 4. 执行所有工作 ---
        d->m_smoothed_delta_ms += (raw_delta_ms - d->m_smoothed_delta_ms) *
                                  d->m_delta_smoothing_factor;
        if (auto mapinfo = static_cast<MapCanvasInfo*>(d->getinfo()); mapinfo) {
            d->canvas_clock.updateWBox(mapinfo->realTimeInfo,
                                       d->m_smoothed_delta_ms);
            // 在这一帧的开始，计算出最终的呈现时间
            // 是一个无状态的、纯粹的变换
            mapinfo->realTimeInfo.current_time_info.presentation_canvas_time =
                mapinfo->realTimeInfo.current_time_info.logic_canvas_time +
                mapinfo->realTimeInfo.offset_info.global_static_offset_ms
                    .load() +
                mapinfo->realTimeInfo.offset_info.global_offset_ms.load();
        }

        d->pre_tickEvent();
        d->layer_manager->sync().startNextFrame();
        d->tickEvent();
        d->layer_manager->sync().waitForAllWorkers();
        d->after_tickEvent();
        d->layer_manager->swapBuffers();

        // --- 5. 发射信号 (跨线程) ---
        emit d->renderUpdate();
    }
}
