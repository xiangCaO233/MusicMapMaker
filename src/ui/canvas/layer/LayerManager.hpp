#ifndef MMM_LAYERMANAGER_HPP
#define MMM_LAYERMANAGER_HPP

#include <QDebug>
#include <QMetaObject>
#include <QThread>
#include <ecs/ECSCore.hpp>
#include <layer/ILayer.hpp>
#include <layer/LayerComputerBase.hpp>
#include <layer/background/BackgroundLayer.hpp>
#include <layer/background/BackgroundLayerGenerator.hpp>
#include <layer/effect/EffectLayer.hpp>
#include <layer/effect/EffectLayerGenerator.hpp>
#include <layer/interact/InteractLayerGenerator.hpp>
#include <layer/interact/RealTimeInteractLayer.hpp>
#include <layer/note/NoteLayer.hpp>
#include <layer/note/NoteLayerGenerator.hpp>
#include <layer/timeline/TimelineLayer.hpp>
#include <layer/timeline/TimelineLayerGenerator.hpp>
#include <memory>
#include <render/synchronize/FrameSynchronizer.hpp>
#include <unordered_map>

class Renderer2D;

class LayerManager {
   public:
    // 构造LayerManager
    explicit LayerManager(Renderer2D* render) : renderer(render) {}

    // 析构LayerManager
    virtual ~LayerManager() {
        // 先发送停止线程信号
        for (const auto& [type, generator] : generators) {
            qDebug() << "生成器[" << static_cast<uint32_t>(type)
                     << "]发送停止信号";
            generator->stop();
        }

        synchronizer.startNextFrame();

        // 等待所有线程退出
        for (const auto& [type, thread] : threads) {
            thread->quit();
            qDebug() << "线程[" << static_cast<uint32_t>(type) << "]等待停止";
            thread->wait();
        }
        // 先清理生成器(内部有个图层指针)
        generators.clear();
        data_buffer.clear();
    }

    // 按图层顺序消费
    template <typename Func>
    void consume(Func f) const {
        // 逐一消费
        for (const auto& [_, buffer] : data_buffer) {
            f(buffer->frontbuffer());
        }
    }

    // 交换所有图层前后缓冲区
    void swapBuffers() const {
        // 逐一翻转
        for (const auto& [_, buffer] : data_buffer) {
            buffer->swapBuffers();
        }
    }

    // 分发信息更新
    void updateInfoForLayers(SharedCanvasInfo* info) const {
        for (const auto& [_, buffer] : data_buffer) {
            buffer->updateInfo(info);
        }
    }

    // 获取同步器
    FrameSynchronizer& sync() { return synchronizer; }

    virtual void initializeLayers() {
        // 初始化所有图层-需重写
    }

    // 渲染数据
    using LayerData = std::map<LayerType, std::unique_ptr<ILayer>>;
    using LayerGenerators =
        std::unordered_map<LayerType, std::unique_ptr<LayerComputerBase>>;

   protected:
    Renderer2D* render() { return renderer; };
    LayerData& layer_data() { return data_buffer; }
    LayerGenerators& layer_generators() { return generators; }
    const LayerGenerators& layer_generators() const { return generators; }

    // 启动生成器
    void startGenerator(LayerType type, LayerComputerBase* layerGenerator) {
        auto thread = std::make_unique<QThread>();
        // 将worker对象“移动”到新线程。这意味着它的所有槽函数和事件都将在新线程中执行。
        layerGenerator->moveToThread(thread.get());
        // 当线程启动时，开始执行worker的run()循环。
        // 这是启动的核心，保证了run()在新线程中被调用。

        QObject::connect(thread.get(), &QThread::started, layerGenerator,
                         &LayerComputerBase::run);
        // 启动线程
        thread->start();

        // 保存对线程和生成器的所有权
        threads.try_emplace(type, std::move(thread));
    }

    // 可重写的更新map接口
    virtual void updateMap(MMap* map) {}

   private:
    // 渲染器
    Renderer2D* renderer;

    // 持有全部图层缓冲区
    LayerData data_buffer;

    // 持有全部图层生成器
    LayerGenerators generators;

    // 持有所有生成器线程的指针
    std::unordered_map<LayerType, std::unique_ptr<QThread>> threads;

    // 持有帧同步器
    FrameSynchronizer synchronizer{5};

    friend class RenderDataLoop;
};

#endif  // MMM_LAYERMANAGER_HPP
