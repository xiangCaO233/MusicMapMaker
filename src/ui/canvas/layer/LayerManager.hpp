#ifndef MMM_LAYERMANAGER_HPP
#define MMM_LAYERMANAGER_HPP

#include <QDebug>
#include <QMetaObject>
#include <QThread>
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

class LayerManager {
   public:
    // 构造LayerManager
    LayerManager() {
        // 初始化渲染数据缓冲区
        using enum LayerType;
        auto background_layer =
            data_buffer
                .try_emplace(BACKGROUND, std::make_unique<BackgroundLayer>())
                .first->second.get();
        auto timeline_layer =
            data_buffer.try_emplace(TIMELINE, std::make_unique<TimelineLayer>())
                .first->second.get();
        auto note_layer =
            data_buffer.try_emplace(NOTE, std::make_unique<NoteLayer>())
                .first->second.get();
        auto effect_layer =
            data_buffer.try_emplace(EFFECT, std::make_unique<EffectLayer>())
                .first->second.get();
        auto interact_layer =
            data_buffer
                .try_emplace(INTERACT,
                             std::make_unique<RealTimeInteractLayer>())
                .first->second.get();

        // 初始化图层生成器
        auto background_generator =
            generators
                .try_emplace(BACKGROUND,
                             std::make_unique<BackgroundLayerGenerator>(
                                 background_layer, &synchronizer))
                .first->second.get();

        auto timeline_generator =
            generators
                .try_emplace(TIMELINE, std::make_unique<TimelineLayerGenerator>(
                                           timeline_layer, &synchronizer))
                .first->second.get();

        auto note_generator =
            generators
                .try_emplace(NOTE, std::make_unique<NoteLayerGenerator>(
                                       note_layer, &synchronizer))
                .first->second.get();

        auto effect_generator =
            generators
                .try_emplace(EFFECT, std::make_unique<EffectLayerGenerator>(
                                         effect_layer, &synchronizer))
                .first->second.get();

        auto interact_generator =
            generators
                .try_emplace(INTERACT, std::make_unique<InteractLayerGenerator>(
                                           interact_layer, &synchronizer))
                .first->second.get();

        startGenerator(BACKGROUND, background_generator);
        startGenerator(TIMELINE, timeline_generator);
        startGenerator(NOTE, note_generator);
        startGenerator(EFFECT, effect_generator);
        startGenerator(INTERACT, interact_generator);
    }
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
    void consume(Func f) {
        // 逐一消费
        for (int i = 0; i <= static_cast<uint32_t>(LayerType::INTERACT); ++i) {
            f(data_buffer[static_cast<LayerType>(i)]->frontbuffer());
        }
    }

    // 交换所有图层前后缓冲区
    void swapBuffers() {
        // 逐一翻转
        for (int i = 0; i <= static_cast<uint32_t>(LayerType::INTERACT); ++i) {
            data_buffer[static_cast<LayerType>(i)]->swapBuffers();
        }
    }

    // 分发信息更新
    void updateInfoForLayers(SharedCanvasInfo* info) {
        for (int i = 0; i <= static_cast<uint32_t>(LayerType::INTERACT); ++i) {
            data_buffer[static_cast<LayerType>(i)]->updateInfo(info);
        }
    }

    // 获取同步器
    FrameSynchronizer& sync() { return synchronizer; }

   private:
    // 渲染数据
    using LayerData = std::unordered_map<LayerType, std::unique_ptr<ILayer>>;
    using LayerGenerators =
        std::unordered_map<LayerType, std::unique_ptr<LayerComputerBase>>;

    // 持有全部图层缓冲区
    LayerData data_buffer;

    // 持有全部图层生成器
    LayerGenerators generators;

    // 持有所有生成器线程的指针
    std::unordered_map<LayerType, std::unique_ptr<QThread>> threads;

    // 持有帧同步器
    FrameSynchronizer synchronizer{2};

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
};

#endif  // MMM_LAYERMANAGER_HPP
