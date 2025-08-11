#ifndef MMM_LAYERCOMPUTERBASE_HPP
#define MMM_LAYERCOMPUTERBASE_HPP

#include <qobject.h>

#include <layer/ILayer.hpp>

class FrameSynchronizer;
class LayerComputerBase : public QObject {
    Q_OBJECT
   public:
    // 构造LayerComputerBase
    LayerComputerBase(ILayer* layer, FrameSynchronizer* sync,
                      QObject* parent = nullptr);
    // 析构LayerComputerBase
    ~LayerComputerBase() override = default;

   public slots:
    void run();

    void stop();

   protected:
    virtual void generateLayer(ILayer::RenderDataBuffer& buffer) {
        // 子类需实现 (=0)
    }

   private:
    // 图层指针
    ILayer* layer;
    // 帧同步器
    FrameSynchronizer* synchronizer;

    std::atomic<bool> isrunning{true};
};

#endif  // MMM_LAYERCOMPUTERBASE_HPP
