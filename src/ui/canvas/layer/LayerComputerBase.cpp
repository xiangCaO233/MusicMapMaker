#include <QDebug>
#include <layer/LayerComputerBase.hpp>
#include <render/synchronize/FrameSynchronizer.hpp>

// 构造LayerComputerBase
LayerComputerBase::LayerComputerBase(LayerManager* manager, ILayer* layerptr,
                                     FrameSynchronizer* sync, QObject* parent)
    : QObject(parent),
      manager_ref(manager),
      layer_ptr(layerptr),
      synchronizer(sync) {}

void LayerComputerBase::run() {
    while (isrunning.load()) {
        // qDebug() << "图层[" << static_cast<uint32_t>(layer_ptr->type()) <<
        // "]"
        //          << "等待开始信号";
        // 在“开始”栅栏处等待，直到主循环发出信号
        synchronizer->workerWaitForFrameStart();

        // 检查是否在等待期间被要求停止
        if (!isrunning.load()) break;

        auto& buffer = layer_ptr->backbuffer();
        buffer.clear();
        generateLayer(manager_ref, buffer);

        // qDebug() << "图层[" << static_cast<uint32_t>(layer_ptr->type()) <<
        // "]"
        //          << "等待其他图层线程完成";
        // 在“完成”栅栏处报告，并等待其他所有worker
        synchronizer->workerFinishedFrame();
    }
    // 结束时也需要在“完成”栅栏处报告
    synchronizer->workerFinishedFrame();
}

void LayerComputerBase::stop() {
    isrunning.store(false);
    // synchronizer->startNextFrame();
}
