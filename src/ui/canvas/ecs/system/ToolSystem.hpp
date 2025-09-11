#ifndef MMM_TOOLSYSTEM_HPP
#define MMM_TOOLSYSTEM_HPP

#include <ecs/component/CoreComponents.hpp>
#include <ecs/component/TransformComponents.hpp>
#include <ecs/system/QuadTree.hpp>
#include <unordered_map>
#include <vector>

struct HitResult {};

class ToolSystem {
   public:
    explicit ToolSystem(entt::registry& reg) : registry(reg) {};
    void update(
        const std::unordered_map<entt::entity, GeneratedMesh>& note_meshs) {
        // 连接 PositionComponent 的创建、更新和销毁信号
        // 当这些事件发生时，实体会被自动加入 m_dirty_entities 列表
        connections.emplace_back(
            registry.on_construct<TimeComponent>()
                .connect<&ToolSystem::mark_as_dirty>(this));
        connections.emplace_back(
            registry.on_update<TimeComponent>()
                .connect<&ToolSystem::mark_as_dirty>(this));
        connections.emplace_back(
            registry.on_destroy<TimeComponent>()
                .connect<&ToolSystem::mark_as_dirty>(this));
    }

    // 更新画布世界箱尺寸
    void update_world_boundbox(BoundingBox box);

    /**
     * @brief [线程安全] UI 线程可以调用此函数进行交互查询.
     *        您需要确保 UI 线程的读取与工作线程的 update 写入之间没有竞争.
     *        一个简单的策略是使用双缓冲，或者在查询时加一个读锁.
     *        (为简化，这里暂不实现锁，但实际应用中必须考虑)
     * @param point 要查询的屏幕坐标.
     * @return 可选的命中结果.
     */
    std::optional<HitResult> query(const glm::vec2& point) const;

   private:
    /**
     * @brief [信号回调] 当 PositionComponent 发生变化时，将实体标记为“脏”.
     */
    void mark_as_dirty(entt::registry&, entt::entity entity);

    /**
     * @brief 移除一个实体的所有旧交互部件.
     * @param entity 要移除的实体.
     */
    void remove_entity_parts(entt::entity entity);

    /**
     * @brief 为一个实体添加新的交互部件.
     * @param entity 实体ID.
     * @param mesh 该实体的新生成网格.
     */
    void add_entity_parts(entt::entity entity, const GeneratedMesh& mesh);

   private:
    // 网格信息仓库
    std::vector<MeshPartInfo> mesh_info_storage;
    // 网格空间索引树
    LooseQuadtree<MeshPartInfo> mesh_info_tree{{}};
    std::unordered_map<entt::entity,
                       std::vector<std::vector<MeshPartInfo>::iterator>>
        entity_to_parts;

    // 对 registry 的引用，用于信号
    entt::registry& registry;
    // --- 变化跟踪 ---
    // 存储自上次 update 以来发生变化的实体
    entt::dense_set<entt::entity> dirty_entities;
    std::vector<entt::connection> connections;
};

#endif  // MMM_TOOLSYSTEM_HPP
