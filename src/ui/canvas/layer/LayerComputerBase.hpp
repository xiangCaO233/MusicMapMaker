#ifndef MMM_LAYERCOMPUTERBASE_HPP
#define MMM_LAYERCOMPUTERBASE_HPP

#include <qobject.h>

#include <layer/ILayer.hpp>
#include <render/command/RenderCommand.hpp>

class FrameSynchronizer;
class LayerManager;

class LayerComputerBase : public QObject {
    Q_OBJECT
   public:
    // 构造LayerComputerBase
    LayerComputerBase(LayerManager* manager, ILayer* layer,
                      FrameSynchronizer* sync, QObject* parent = nullptr);
    // 析构LayerComputerBase
    ~LayerComputerBase() override = default;

    // 获取对应图层
    template <typename Layer>
    Layer* layer() {
        return static_cast<Layer*>(layer_ptr);
    }

   public slots:
    void run();

    void stop();

   protected:
    // 生成图层
    virtual void generateLayer(LayerManager* manager,
                               RenderDataBuffer& buffer) = 0;

   private:
    // 图层指针
    ILayer* layer_ptr;

    // 图层管理器引用
    LayerManager* manager_ref;

    // 帧同步器
    FrameSynchronizer* synchronizer;

    // 是否运行
    std::atomic<bool> isrunning{true};
};

#endif  // MMM_LAYERCOMPUTERBASE_HPP
