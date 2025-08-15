#ifndef MMM_MAPDATALOOP_HPP
#define MMM_MAPDATALOOP_HPP

#include <render/synchronize/tick/RenderDataLoop.hpp>

class MapDataLoop : public RenderDataLoop {
    Q_OBJECT
   public:
    // 构造MapDataLoop
    using RenderDataLoop::RenderDataLoop;
    // 析构MapDataLoop
    ~MapDataLoop() override = default;

    // 初始化层管理器
    void initializeLayerManager() override;

   protected:
    // 可重写的tick事件(执行其他任务)
    void tickEvent() override;

    // 可重写的更新map接口
    // void updateMap(MMap* map) override {}
};
#endif  // MMM_MAPDATALOOP_HPP
