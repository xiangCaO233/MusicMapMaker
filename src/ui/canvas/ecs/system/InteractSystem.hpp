#ifndef MMM_INTERACTSYSTEM_HPP
#define MMM_INTERACTSYSTEM_HPP

#include <deque>
#include <ecs/ECSCore.hpp>
#include <ecs/component/CoreComponents.hpp>
#include <ecs/component/NoteComponents.hpp>
#include <ecs/component/TransformComponents.hpp>
#include <ecs/system/TimePixelConverter.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/vector_angle.hpp>
#include <info/MapCanvasInfo.hpp>
#include <layer/ILayer.hpp>
#include <map/skin/MSkin.hpp>

class InteractSystem {
   public:
    // 构造函数，在这里初始化我们的“现实时钟”
    InteractSystem()
        : m_start_time(std::chrono::high_resolution_clock::now()) {}
    void update(ECSCore& core, const MapCanvasInfo* info, ILayer* layer,
                RenderDataBuffer& buffer) const {}

   private:
    // --- 内部状态，用于存储鼠标轨迹 ---
    struct MouseTrackPoint {
        glm::vec2 pos;
        double timestamp_ms;  // 记录该点的时间戳
    };
    // 使用 mutable 关键字，因为即使在 const 的 update 方法中，
    // 也需要修改这个轨迹数据。这在逻辑上是合理的，
    // 因为轨迹是系统的内部状态，而不是外部传入的数据。
    mutable std::deque<MouseTrackPoint> m_mouse_track;
    // 将计时器起点作为成员变量，确保它只被初始化一次
    const std::chrono::time_point<std::chrono::high_resolution_clock>
        m_start_time;
};

#endif  // MMM_INTERACTSYSTEM_HPP
