#ifndef MMM_NOTEIDMANAGER_HPP
#define MMM_NOTEIDMANAGER_HPP

#include <assert.h>

#include <mmm/ObjectHandle.hpp>
/**
 * @class NoteIDManager
 * @brief 维护稳定ID (StableNoteID) 和易变句柄 (NoteHandle) 之间的双向映射。
 * @details
 * 这是连接应用程序逻辑（使用稳定ID）和底层数据存储（使用易变句柄）的关键桥梁，
 *          使得撤销/重做操作能够正确地处理句柄变化。
 */
class NoteIDManager {
   public:
    NoteIDManager() = default;

    /**
     * @brief 为一个新创建的 Note 注册一个稳定ID。
     * @param volatile_handle 从 SlottedArray 获取的初始 NoteHandle。
     * @return 新生成的、唯一的 StableNoteID。
     */
    NoteUUID register_new_note(NoteHandle volatile_handle) {
        // 在 Debug 模式下断言，确保我们不会重复注册同一个 handle。
        assert(m_volatile_to_stable.find(volatile_handle) ==
                   m_volatile_to_stable.end() &&
               "Attempted to register an already existing handle.");

        NoteUUID id = m_next_id++;
        m_stable_to_volatile[id] = volatile_handle;
        m_volatile_to_stable[volatile_handle] = id;
        return id;
    }

    /**
     * @brief 当一个 Note 被永久删除时，注销其ID和句柄。
     * @param id 要移除的 Note 的 StableNoteID。
     */
    void remove_note(NoteUUID id) {
        auto it = m_stable_to_volatile.find(id);
        if (it != m_stable_to_volatile.end()) {
            NoteHandle handle_to_remove = it->second;
            m_volatile_to_stable.erase(handle_to_remove);
            m_stable_to_volatile.erase(it);
        }
    }

    /**
     * @brief 在撤销/重做操作后，更新一个稳定ID对应的句柄。
     * @param id 保持不变的 StableNoteID。
     * @param new_handle Note 被恢复后获得的新 NoteHandle。
     */
    void update_handle(NoteUUID id, NoteHandle new_handle) {
        auto it = m_stable_to_volatile.find(id);
        // 断言确保我们正在更新一个已存在的ID
        assert(it != m_stable_to_volatile.end() &&
               "Attempted to update a handle for a non-existent StableNoteID.");
        if (it == m_stable_to_volatile.end()) return;

        // 1. 获取旧的句柄
        NoteHandle old_handle = it->second;

        // 2. 如果新旧句柄不同，才需要更新
        if (old_handle == new_handle) return;

        // 3. 从反向映射中移除旧句柄
        m_volatile_to_stable.erase(old_handle);

        // 4. 更新正向映射
        it->second = new_handle;

        // 5. 在反向映射中添加新句柄
        m_volatile_to_stable[new_handle] = id;
    }

    /**
     * @brief 通过稳定ID查找当前的临时句柄。
     * @param id 要查询的 StableNoteID。
     * @return 对应的 NoteHandle，如果ID不存在则返回一个无效句柄。
     */
    NoteHandle get_handle(NoteUUID id) const {
        auto it = m_stable_to_volatile.find(id);
        if (it != m_stable_to_volatile.end()) {
            return it->second;
        }
        return NoteHandle::invalid();
    }

    /**
     * @brief 通过临时句柄反向查找其稳定ID。
     * @param handle 要查询的 NoteHandle。
     * @return 对应的 StableNoteID，如果句柄未注册则返回 InvalidStableNoteID。
     */
    NoteUUID get_id(NoteHandle handle) const {
        auto it = m_volatile_to_stable.find(handle);
        if (it != m_volatile_to_stable.end()) {
            return it->second;
        }
        return InvalidNoteUUID;
    }

    /**
     * @brief 检查一个稳定ID是否已被注册。
     */
    bool contains(NoteUUID id) const {
        return m_stable_to_volatile.count(id) > 0;
    }

    /**
     * @brief 清空所有映射关系。
     */
    void clear() {
        m_stable_to_volatile.clear();
        m_volatile_to_stable.clear();
        m_next_id = 1;  // 重置ID计数器
    }

   private:
    std::unordered_map<NoteUUID, NoteHandle> m_stable_to_volatile;
    std::unordered_map<NoteHandle, NoteUUID, NoteHandle::Hash>
        m_volatile_to_stable;
    NoteUUID m_next_id = 1;  // 从1开始，因为0被定义为无效ID
};

#endif  // MMM_NOTEIDMANAGER_HPP
