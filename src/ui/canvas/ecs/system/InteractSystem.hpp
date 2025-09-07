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
    // 初始化现实时钟
    InteractSystem() = default;

    void update(const ECSCore& core, const MapCanvasInfo* info, ILayer* layer,
                RenderDataBuffer& buffer) const {
        // --- 1. 更新鼠标轨迹 ---
        auto now = std::chrono::high_resolution_clock::now();
        const double current_real_time_ms =
            std::chrono::duration<double, std::milli>(now - m_start_time)
                .count();

        const auto& realtime_info = info->realTimeInfo;
        const glm::vec2 current_mouse_pos = {realtime_info.mousePos.x(),
                                             realtime_info.mousePos.y()};

        // 为了平滑，即使鼠标不动也持续添加点，但pop的逻辑会处理掉它们
        if (m_mouse_track.empty() ||
            m_mouse_track.front().pos != current_mouse_pos) {
            m_mouse_track.push_front({current_mouse_pos, current_real_time_ms});
        }

        const double trail_duration_ms = 300.0;
        while (m_mouse_track.size() > 2 &&
               (current_real_time_ms - m_mouse_track.back().timestamp_ms >
                trail_duration_ms)) {
            m_mouse_track.pop_back();
        }

        if (m_mouse_track.size() <= 2) return;

        // --- 2. 定义外观和精度 ---
        const float start_width = 24.0f;
        const float end_width = 2.0f;
        const float start_alpha = 1.f;
        const float end_alpha = 1.f;
        const int joint_segments = 32;  // 圆形关节的平滑度

        // --- 3. 创建一个MeshCommand来容纳所有几何体 ---
        MeshCommand trail_cmd;
        trail_cmd.cmdType = CommandType::MESH;
        // 重要：为了正确的Alpha混合，你需要告诉渲染器这个Mesh不需要深度写入
        // 你可能需要给MeshCommand加一个标志，或者在渲染器中特殊处理
        // trail_cmd.depth_write_enabled = false;

        // --- 4. 生成所有连接的四边形 (身体) ---
        for (size_t i = 0; i < m_mouse_track.size() - 1; ++i) {
            const auto& p1 = m_mouse_track[i];
            const auto& p2 = m_mouse_track[i + 1];

            double age1 =
                (current_real_time_ms - p1.timestamp_ms) / trail_duration_ms;
            float width1 =
                glm::mix(start_width, end_width, static_cast<float>(age1));
            glm::vec4 color1 = {
                1.f, 1.f, 1.f,
                glm::mix(start_alpha, end_alpha, static_cast<float>(age1))};

            double age2 =
                (current_real_time_ms - p2.timestamp_ms) / trail_duration_ms;
            float width2 =
                glm::mix(start_width, end_width, static_cast<float>(age2));
            glm::vec4 color2 = {
                1.f, 1.f, 1.f,
                glm::mix(start_alpha, end_alpha, static_cast<float>(age2))};

            generateSegment(trail_cmd, p1.pos, p2.pos, width1, width2, color1,
                            color2);
        }

        // --- 5. 生成所有圆形的关节 (覆盖尖角) ---
        for (size_t i = 0; i < m_mouse_track.size(); ++i) {
            const auto& p = m_mouse_track[i];

            double age =
                (current_real_time_ms - p.timestamp_ms) / trail_duration_ms;
            float width =
                glm::mix(start_width, end_width, static_cast<float>(age));
            glm::vec4 color = {
                1.f, 1.f, 1.f,
                glm::mix(start_alpha, end_alpha, static_cast<float>(age))};

            generateRoundJoint(trail_cmd, p.pos, width, color, joint_segments);
        }

        if (!trail_cmd.vertices.empty()) {
            buffer.add_MeshCommand(trail_cmd);
        }

        // 检测鼠标是否悬浮在某物件上
    }

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
        m_start_time{std::chrono::high_resolution_clock::now()};
    // --- 几何生成辅助函数 ---

    /**
     * @brief 为两个点之间生成一个四边形（2个三角形）
     */
    void generateSegment(MeshCommand& cmd, const glm::vec2& p1,
                         const glm::vec2& p2, float w1, float w2,
                         const glm::vec4& c1, const glm::vec4& c2) const {
        glm::vec2 dir = p1 - p2;
        if (glm::length(dir) < 1e-6) return;
        dir = glm::normalize(dir);
        glm::vec2 normal = {dir.y, -dir.x};

        size_t base_index = cmd.vertices.size();

        cmd.vertices.emplace_back(p1 + normal * (w1 / 2.f), glm::vec2{0.f, 0.f},
                                  c1);  // p1-left
        cmd.vertices.emplace_back(p1 - normal * (w1 / 2.f), glm::vec2{1.f, 0.f},
                                  c1);  // p1-right
        cmd.vertices.emplace_back(p2 + normal * (w2 / 2.f), glm::vec2{0.f, 1.f},
                                  c2);  // p2-left
        cmd.vertices.emplace_back(p2 - normal * (w2 / 2.f), glm::vec2{1.f, 1.f},
                                  c2);  // p2-right

        cmd.indicies.push_back(base_index);
        cmd.indicies.push_back(base_index + 2);
        cmd.indicies.push_back(base_index + 1);

        cmd.indicies.push_back(base_index + 1);
        cmd.indicies.push_back(base_index + 2);
        cmd.indicies.push_back(base_index + 3);
    }

    /**
     * @brief 在一个点上生成一个由多个三角形组成的圆形
     */
    void generateRoundJoint(MeshCommand& cmd, const glm::vec2& center,
                            float width, const glm::vec4& color,
                            int segments) const {
        size_t center_index = cmd.vertices.size();
        cmd.vertices.emplace_back(center, glm::vec2{0.5f, 0.5f},
                                  color);  // 中心点 (UV设为0.5,0.5)

        float radius = width / 2.0f;

        // 生成圆周上的顶点
        for (int i = 0; i < segments; ++i) {
            float angle = 2.0f * glm::pi<float>() * static_cast<float>(i) /
                          static_cast<float>(segments);
            glm::vec2 point_on_circle =
                center + glm::vec2(std::cos(angle), std::sin(angle)) * radius;
            cmd.vertices.emplace_back(point_on_circle, glm::vec2{0.f, 0.f},
                                      color);
        }

        // 生成连接中心点和圆周点的三角形索引
        for (int i = 1; i <= segments; ++i) {
            cmd.indicies.push_back(center_index);
            cmd.indicies.push_back(center_index + i);
            // 连接到下一个点，最后一个点连接回第一个圆周点
            size_t next_v_idx =
                (i == segments) ? center_index + 1 : center_index + i + 1;
            cmd.indicies.push_back(next_v_idx);
        }
    }
};

#endif  // MMM_INTERACTSYSTEM_HPP
