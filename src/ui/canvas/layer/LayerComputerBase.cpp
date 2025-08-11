#include <layer/LayerComputerBase.hpp>
#include <render/synchronize/FrameSynchronizer.hpp>

// 构造LayerComputerBase
LayerComputerBase::LayerComputerBase(ILayer* layerptr, FrameSynchronizer* sync,
                                     QObject* parent)
    : QObject(parent), layer(layerptr), synchronizer(sync) {}

void LayerComputerBase::run() {
    while (isrunning) {
        // 在“开始”栅栏处等待，直到主循环发出信号
        synchronizer->workerWaitForFrameStart();

        // 检查是否在等待期间被要求停止
        if (!isrunning) break;

        auto& buffer = layer->backbuffer();
        buffer.clear();
        generateLayer(buffer);

        // 在“完成”栅栏处报告，并等待其他所有worker
        synchronizer->workerFinishedFrame();
    }
}

void LayerComputerBase::stop() { isrunning = false; }
