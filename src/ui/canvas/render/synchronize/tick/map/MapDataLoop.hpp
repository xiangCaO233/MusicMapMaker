#ifndef MMM_MAPDATALOOP_HPP
#define MMM_MAPDATALOOP_HPP

#include <ecs/system/TimeSystem.hpp>
#include <ecs/system/sync/SyncSystem.hpp>
#include <render/synchronize/tick/RenderDataLoop.hpp>

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

   private:
    [[no_unique_address]] SyncSystem sync_system;
    [[no_unique_address]] TimeSystem time_system;
};
#endif  // MMM_MAPDATALOOP_HPP
