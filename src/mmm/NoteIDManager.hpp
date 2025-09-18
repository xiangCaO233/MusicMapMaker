#ifndef MMM_NOTEIDMANAGER_HPP
#define MMM_NOTEIDMANAGER_HPP

#include <assert.h>

#include <mmm/ObjectHandle.hpp>

/**
 * @class NoteIDManager
 * @brief 维护UUID (NoteUUID) 和易变句柄 (NoteHandle) 之间的双向映射。
 * @details
 * 这是连接应用程序逻辑（使用UUID）和底层数据存储（使用易变句柄）的关键桥梁，
 *          使得撤销/重做操作能够正确地处理句柄变化。
 */
class NoteIDManager {
   public:
    NoteIDManager() = default;

    /**
     * @brief 为一个新创建的 Note 注册一个UUID。
     * @param volatile_handle 从 SlottedArray 获取的初始 NoteHandle。
     * @return 新生成的、唯一的 UUID。
     */
    NoteUUID register_new_note(NoteHandle volatile_handle) {
        // 在 Debug 模式下断言，确保我们不会重复注册同一个 handle。
        assert(m_volatile_to_uuid.find(volatile_handle) ==
                   m_volatile_to_uuid.end() &&
               "Attempted to register an already existing handle.");

        NoteUUID id = m_next_id++;
        m_uuid_to_volatile[id] = volatile_handle;
        m_volatile_to_uuid[volatile_handle] = id;
        return id;
    }

    /**
     * @brief 当一个 Note 被永久删除时，注销其ID和句柄。
     * @param id 要移除的 Note 的 UUID。
     */
    void remove_note(NoteUUID id) {
        auto it = m_uuid_to_volatile.find(id);
        if (it != m_uuid_to_volatile.end()) {
            NoteHandle handle_to_remove = it->second;
            m_volatile_to_uuid.erase(handle_to_remove);
            m_uuid_to_volatile.erase(it);
        }
    }

    /**
     * @brief 在撤销/重做操作后，更新一个UUID对应的句柄。
     * @param id 保持不变的 UUID
     * @param new_handle Note 被恢复后获得的新 NoteHandle。
     */
    void update_handle(NoteUUID id, NoteHandle new_handle) {
        // 1. 检查这个ID是否已经有一个旧的句柄需要被清理
        auto it = m_uuid_to_volatile.find(id);
        if (it != m_uuid_to_volatile.end()) {
            // 如果存在旧句柄，从反向映射中移除它
            m_volatile_to_uuid.erase(it->second);
        }

        // 2. 建立新的映射关系（对于 map 的下标运算符，这既是插入也是更新）
        m_uuid_to_volatile[id] = new_handle;
        m_volatile_to_uuid[new_handle] = id;
    }

    /**
     * @brief 通过UUID查找当前的临时句柄
     * @param id 要查询的 UUID
     * @return 对应的 NoteHandle，如果ID不存在则返回一个无效句柄
     */
    NoteHandle get_handle(NoteUUID id) const {
        auto it = m_uuid_to_volatile.find(id);
        if (it != m_uuid_to_volatile.end()) {
            return it->second;
        }
        return NoteHandle::invalid();
    }

    /**
     * @brief 通过临时句柄反向查找其UUID。
     * @param handle 要查询的 NoteHandle。
     * @return 对应的 UUID，如果句柄未注册则返回 InvalidNoteUUID。
     */
    NoteUUID get_id(NoteHandle handle) const {
        auto it = m_volatile_to_uuid.find(handle);
        if (it != m_volatile_to_uuid.end()) {
            return it->second;
        }
        return InvalidNoteUUID;
    }

    /**
     * @brief 检查一个UUID是否已被注册。
     */
    bool contains(NoteUUID id) const {
        return m_uuid_to_volatile.count(id) > 0;
    }

    /**
     * @brief 清空所有映射关系。
     */
    void clear() {
        m_uuid_to_volatile.clear();
        m_volatile_to_uuid.clear();
        m_next_id = 1;  // 重置ID计数器
    }

   private:
    std::unordered_map<NoteUUID, NoteHandle> m_uuid_to_volatile;
    std::unordered_map<NoteHandle, NoteUUID, NoteHandle::Hash>
        m_volatile_to_uuid;
    // 从1开始，因为0被定义为无效ID
    NoteUUID m_next_id = 1;
};

#endif  // MMM_NOTEIDMANAGER_HPP
