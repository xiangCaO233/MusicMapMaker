#ifndef MMM_TOOLSYSTEM_HPP
#define MMM_TOOLSYSTEM_HPP

#include <QDebug>
#include <ecs/component/CoreComponents.hpp>
#include <ecs/component/NoteComponents.hpp>
#include <ecs/component/TransformComponents.hpp>
#include <ecs/system/QuadTree.hpp>
#include <info/MapCanvasInfo.hpp>
#include <mmm/ObjectHandle.hpp>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>

class ToolSystem {
   public:
    explicit ToolSystem(entt::registry& reg) : registry(reg) {}
    ~ToolSystem() = default;

    /**
     * @brief [工作线程] 增量更新空间索引.
     * @param generated_meshes 当前帧所有可见实体的完整网格数据.
     */
    void update(
        const std::unordered_map<entt::entity, GeneratedMesh>& note_meshs,
        const MapCanvasInfo* info) {
        // 使用 std::unique_lock 获取独占的写锁
        std::unique_lock<std::shared_mutex> lock(mtx);

        // 使用四叉树实现的移动赋值运算符，基于最新的世界边界创建一个全新的空树
        mesh_info_tree =
            LooseQuadtree<MeshPartInfo>({{0, 0},
                                         {info->baseInfo.canvasSize.width(),
                                          info->baseInfo.canvasSize.height()}});

        // 遍历传入的本帧所有网格
        for (const auto& [entity, mesh] : note_meshs) {
            const auto& [track, uuid] = registry.get<NoteComponent>(entity);
            // 遍历网格中的每一个部件 (Quad)
            for (const auto& quad : mesh.mesh) {
                // 获取指向刚刚创建的、位于内存池末尾的对象的指针
                MeshPartInfo new_part_ptr{quad.pos,  quad.size,   entity,
                                          quad.part, quad.zIndex, uuid};

                // 将这个指针插入到全新的四叉树中
                mesh_info_tree.insert(new_part_ptr);
            }
        }
    }

    // LooseQuadtree<MeshPartInfo>& get_mesh_info_tree() { return
    // mesh_info_tree; } const entt::registry& get_registry() { return registry;
    // }

    // 更新画布世界碰撞箱尺寸
    // void update_world_boundbox(BoundingBox box) const {
    //     // 这个操作会改变整个系统的状态，需要获取写锁
    //     // 获取独占的写锁
    //     std::unique_lock<std::shared_mutex> lock(mtx);

    //     // 1. 从当前的树中提取出所有已存储的 MeshPartInfo 对象。
    //     //    这是一个移动操作，旧树在逻辑上变空了。
    //     std::vector<MeshPartInfo> existing_parts =
    //         mesh_info_tree.extract_all_objects();

    //     // 2. 用新的边界框创建一个全新的四叉树。
    //     //    旧树的内存会被自动释放和替换。
    //     mesh_info_tree = LooseQuadtree<MeshPartInfo>(box);

    //     // 3. 将之前提取的所有几何部件重新插入到新的四叉树中。
    //     for (auto& part : existing_parts) {
    //         // 使用 std::move 将部件移动到新树中，避免拷贝
    //         mesh_info_tree.insert(std::move(part));
    //     }
    // }

    /**
     * @brief [线程安全] UI 线程可以调用此函数进行交互查询.
     *        您需要确保 UI 线程的读取与工作线程的 update 写入之间没有竞争.
     *        一个简单的策略是使用双缓冲，或者在查询时加一个读锁.
     * @param point 要查询的屏幕坐标.
     * @return 可选的命中结果.
     */
    std::optional<MeshPartInfo> query(const glm::vec2& point) const {
        // 使用 std::shared_lock 获取共享的读锁
        std::shared_lock<std::shared_mutex> lock(mtx);
        auto candidates = mesh_info_tree.query(point);
        if (candidates.empty()) return std::nullopt;

        // 筛选最上层的匹配项
        std::optional<MeshPartInfo> best_candidate;
        int min_z_index = -1;
        for (const auto& part : candidates) {
            if (part.contains(point)) {
                if (part.zIndex > min_z_index) {
                    min_z_index = part.zIndex;
                    best_candidate = part;
                }
            }
        }
        if (best_candidate) {
            return best_candidate;
        }
        return std::nullopt;
    }

   private:
    // 网格空间索引树
    mutable LooseQuadtree<MeshPartInfo> mesh_info_tree{{}};
    // 读写锁
    mutable std::shared_mutex mtx;
    // 对 registry 的引用
    entt::registry& registry;
};

#endif  // MMM_TOOLSYSTEM_HPP
