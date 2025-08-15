#ifndef MMM_MAPDATALOOP_HPP
#define MMM_MAPDATALOOP_HPP

#include <render/synchronize/tick/RenderDataLoop.hpp>

#include "ecs/system/SyncSystem.hpp"

class SharedCanvasInfo;

class MapDataLoop : public RenderDataLoop {
    Q_OBJECT
   public:
    // 构造MapDataLoop
    using RenderDataLoop::RenderDataLoop;
    // 析构MapDataLoop
    ~MapDataLoop() override = default;

    // 初始化层管理器
    void initializeLayerManager() override;

    void updateCanvasInfo();

   protected:
    // 可重写的tick事件(执行其他任务)
    void pre_tickEvent() override;
    void tickEvent() override;
    void after_tickEvent() override;

    // 可重写的更新map接口
    // void updateMap(MMap* map) override {}
   private:
    SyncSystem sync_system;
};
#endif  // MMM_MAPDATALOOP_HPP
