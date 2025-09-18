#ifndef MMM_OPERATIONCOMMAND_HPP
#define MMM_OPERATIONCOMMAND_HPP

#include <QDebug>
#include <mmm/DataStructures.hpp>
#include <mmm/NoteIDManager.hpp>
#include <mmm/map/editor/MMapEditEvent.hpp>
#include <mmm/timing/Timing.hpp>
#include <tool/ThreadSafeQueue.hpp>
#include <unordered_set>

class OperationCommand {
   public:
    virtual ~OperationCommand() = default;
    virtual bool execute(ThreadSafeQueue<MMapEditEvent>& editEventQueue) = 0;
    virtual void undo(ThreadSafeQueue<MMapEditEvent>& editEventQueue) = 0;
};

class AddTimingPointCommand : public OperationCommand {
   public:
    AddTimingPointCommand(TimingMap& timing_map,
                          std::unique_ptr<Timing> timing_to_add)
        : m_timing_map(timing_map),
          m_timing_data(std::move(timing_to_add)),
          m_added_timing_ptr(nullptr) {}

    bool execute(ThreadSafeQueue<MMapEditEvent>& editEventQueue) override {
        // 如果数据已经被移走（例如在 redo 之后又 undo），则直接返回失败
        if (!m_timing_data) return false;

        // 调用 map 的 add 方法，所有权从命令转移到 map
        m_added_timing_ptr =
            m_timing_map.add_timing_point(std::move(m_timing_data));

        if (m_added_timing_ptr) {
            // 操作成功，发布事件
            editEventQueue.push(
                {MMapEditEventType::TimingAdded, m_added_timing_ptr});
            return true;
        }

        // 添加失败，可能因为重复。我们需要取回所有权。
        // （这需要 add_timing_point 在失败时能返回
        // unique_ptr，或者我们就不支持失败情况） 为了简化，我们假设 add 失败时
        // timing_data 不会被销毁。
        return false;
    }

    void undo(ThreadSafeQueue<MMapEditEvent>& editEventQueue) override {
        // 如果没有记录下成功添加的对象的指针，则无法撤销
        if (!m_added_timing_ptr) return;

        // 调用 map 的 remove 方法，所有权从 map 转移回命令的 m_timing_data 中
        m_timing_data = m_timing_map.remove_timing_point(m_added_timing_ptr);

        if (m_timing_data) {
            // 撤销成功，发布事件
            editEventQueue.push(
                {MMapEditEventType::TimingRemoved, m_added_timing_ptr});
        }

        // 清空指针，因为对象已不在 map 中，此指针不再有效
        m_added_timing_ptr = nullptr;
    }

   private:
    TimingMap& m_timing_map;
    // 用于存储 Timing 数据的 unique_ptr。所有权在 execute 和 undo 之间转移。
    std::unique_ptr<Timing> m_timing_data;
    // 指向被添加到 map 中的那个对象的非拥有指针，用于在 undo 时精确定位。
    Timing* m_added_timing_ptr;
};

class RemoveTimingPointCommand : public OperationCommand {
   public:
    // 构造函数接收一个裸指针，用于识别要删除的目标
    RemoveTimingPointCommand(TimingMap& timing_map, Timing* timing_to_remove)
        : m_timing_map(timing_map), m_timing_to_remove_ptr(timing_to_remove) {}

    bool execute(ThreadSafeQueue<MMapEditEvent>& editEventQueue) override {
        if (!m_timing_to_remove_ptr) return false;

        // 调用 map 的 remove 方法，所有权从 map 转移到命令的备份成员中
        m_removed_timing_backup =
            m_timing_map.remove_timing_point(m_timing_to_remove_ptr);

        if (m_removed_timing_backup) {
            // 操作成功，发布事件
            editEventQueue.push(
                {MMapEditEventType::TimingRemoved, m_timing_to_remove_ptr});
            return true;
        }
        return false;
    }

    void undo(ThreadSafeQueue<MMapEditEvent>& editEventQueue) override {
        // 如果没有备份数据，说明 execute 未成功或已撤销
        if (!m_removed_timing_backup) return;

        // 调用 map 的 add 方法，所有权从命令的备份成员转移回 map
        Timing* restored_ptr =
            m_timing_map.add_timing_point(std::move(m_removed_timing_backup));

        if (restored_ptr) {
            // 撤销成功，发布事件
            editEventQueue.push({MMapEditEventType::TimingAdded, restored_ptr});
        }
    }

   private:
    TimingMap& m_timing_map;
    // 用于识别要删除的对象的非拥有指针
    Timing* m_timing_to_remove_ptr;
    // 用于存储被删除对象数据的备份，以便 undo
    std::unique_ptr<Timing> m_removed_timing_backup;
};

class UpdateTimingCommand : public OperationCommand {
   public:
    // 构造函数接收一个裸指针用于识别目标，以及包含新数据的 unique_ptr
    UpdateTimingCommand(TimingMap& timing_map, Timing* timing_to_update,
                        std::unique_ptr<Timing> new_data)
        : m_timing_map(timing_map),
          m_timing_to_update_ptr(timing_to_update),
          m_new_data(std::move(new_data)) {}

    bool execute(ThreadSafeQueue<MMapEditEvent>& editEventQueue) override {
        if (!m_timing_to_update_ptr || !m_new_data) return false;

        // 调用 map 的 update 方法，将新数据的所有权交给 map，
        // 同时接管 map 返回的旧数据的所有权作为备份。
        m_old_data_backup = m_timing_map.update_timing_point(
            m_timing_to_update_ptr, std::move(m_new_data));

        if (m_old_data_backup) {
            // 操作成功，发布事件
            editEventQueue.push(
                {MMapEditEventType::TimingUpdated, m_timing_to_update_ptr});
            return true;
        }
        return false;
    }

    void undo(ThreadSafeQueue<MMapEditEvent>& editEventQueue) override {
        if (!m_timing_to_update_ptr || !m_old_data_backup) return;

        // 再次调用 map 的 update 方法，但这次是用备份的旧数据去替换当前数据。
        // 当前数据（即之前的新数据）的所有权被返回，并存入 m_new_data 以备
        // redo。
        m_new_data = m_timing_map.update_timing_point(
            m_timing_to_update_ptr, std::move(m_old_data_backup));

        if (m_new_data) {
            // 撤销成功，也发布更新事件，因为对象状态同样发生了变化
            editEventQueue.push(
                {MMapEditEventType::TimingUpdated, m_timing_to_update_ptr});
        }
    }

   private:
    TimingMap& m_timing_map;
    // 用于识别要更新的对象的非拥有指针
    Timing* m_timing_to_update_ptr;
    // 用于存储被替换掉的旧数据的备份
    std::unique_ptr<Timing> m_old_data_backup;
    // 用于存储要应用的新数据
    std::unique_ptr<Timing> m_new_data;
};

class RemoveTimingsAtPointCommand : public OperationCommand {
   public:
    RemoveTimingsAtPointCommand(TimingMap& timing_map, int32_t timestamp)
        : m_timing_map(timing_map), m_timestamp(timestamp) {}

    bool execute(ThreadSafeQueue<MMapEditEvent>& editEventQueue) override {
        // 在执行前清空备份，以支持 redo
        m_removed_timings_backup.clear();

        // 调用 map 的方法，它返回一个包含所有被删除对象所有权的 optional
        auto removed_data_opt =
            m_timing_map.remove_timings_at_point(m_timestamp);

        if (removed_data_opt.has_value()) {
            m_removed_timings_backup = std::move(*removed_data_opt);

            // 为每个被删除的对象发布事件
            for (const auto& removed_timing : m_removed_timings_backup) {
                editEventQueue.push(
                    {MMapEditEventType::TimingRemoved, removed_timing.get()});
            }
            return true;
        }
        return false;
    }

    void undo(ThreadSafeQueue<MMapEditEvent>& editEventQueue) override {
        if (m_removed_timings_backup.empty()) return;

        // 遍历备份（必须使用非 const 引用以便 std::move）
        for (auto& timing_data : m_removed_timings_backup) {
            if (timing_data) {
                // 将所有权交还给 map，并获取指向新添加对象的指针
                Timing* restored_ptr =
                    m_timing_map.add_timing_point(std::move(timing_data));
                if (restored_ptr) {
                    // 为每个恢复的对象发布事件
                    editEventQueue.push(
                        {MMapEditEventType::TimingAdded, restored_ptr});
                }
            }
        }
        // 清空备份，因为所有权已全部移交
        m_removed_timings_backup.clear();
    }

   private:
    TimingMap& m_timing_map;
    int32_t m_timestamp;
    // 存储一批被删除对象的所有权
    std::vector<std::unique_ptr<Timing>> m_removed_timings_backup;
};

class UpdateNoteCommand : public OperationCommand {
   public:
    // 构造函数现在接收 StableNoteID
    UpdateNoteCommand(NoteCollection& collection, NoteIDManager& id_manager,
                      NoteUUID id, std::unique_ptr<Note> new_data)
        : m_collection(collection),
          m_id_manager(id_manager),
          m_id(id),
          m_new_data(std::move(new_data)) {}

    bool execute(ThreadSafeQueue<MMapEditEvent>& editEventQueue) override {
        if (!m_new_data) return false;

        // 动态地从稳定ID获取当前句柄
        NoteHandle current_handle = m_id_manager.get_handle(m_id);
        if (!current_handle.isValid()) return false;

        // 执行更新，并保存返回的“旧数据”，以便 undo
        m_old_data =
            m_collection.update_note(current_handle, std::move(m_new_data));
        auto success = m_old_data != nullptr;
        if (success) {
            // 发布事件
            editEventQueue.push({MMapEditEventType::NoteUpdated, m_id});
        }
        return success;
    }

    void undo(ThreadSafeQueue<MMapEditEvent>& editEventQueue) override {
        if (!m_old_data) return;

        // 同样，动态获取句柄
        NoteHandle current_handle = m_id_manager.get_handle(m_id);
        if (!current_handle.isValid()) return;

        // 撤销更新，就是用旧数据再更新一次
        m_new_data =
            m_collection.update_note(current_handle, std::move(m_old_data));

        // 发布事件
        editEventQueue.push({MMapEditEventType::NoteUpdated, m_id});
    }

   private:
    NoteCollection& m_collection;
    NoteIDManager& m_id_manager;  // 增加了对 ID 管理器的引用
    NoteUUID m_id;                // 存储稳定ID，而不是 NoteHandle
    std::unique_ptr<Note> m_old_data;
    std::unique_ptr<Note> m_new_data;
};

class AddNoteCommand : public OperationCommand {
   public:
    AddNoteCommand(NoteCollection& collection, NoteIDManager& id_manager,
                   std::unique_ptr<Note> note)
        : m_collection(collection),
          m_id_manager(id_manager),
          m_note_to_add(std::move(note)),
          m_id(InvalidNoteUUID) {}  // 初始时没有稳定ID

    bool execute(ThreadSafeQueue<MMapEditEvent>& editEventQueue) override {
        // Redo 逻辑: 如果是重做，从备份中恢复 note 数据
        if (!m_note_to_add && m_note_backup_for_redo) {
            m_note_to_add = std::move(m_note_backup_for_redo);
        }
        if (!m_note_to_add) return false;

        NoteHandle new_handle = m_collection.add_note(std::move(m_note_to_add));
        if (!new_handle.isValid()) return false;

        if (m_id == InvalidNoteUUID) {
            // 首次执行 (Execute): 注册一个全新的稳定 ID
            m_id = m_id_manager.register_new_note(new_handle);
        } else {
            // 重做 (Redo): 此时 m_id 是已知的，我们必须使用 update_handle
            // 来恢复映射
            m_id_manager.update_handle(m_id, new_handle);
        }
        // 发布事件
        editEventQueue.push({MMapEditEventType::NoteAdded, m_id});
        return true;
    }

    void undo(ThreadSafeQueue<MMapEditEvent>& editEventQueue) override {
        // 通过稳定 ID 获取当前的句柄
        NoteHandle current_handle = m_id_manager.get_handle(m_id);
        if (!current_handle.isValid()) return;

        // 移除 Note 并备份数据，以便 Redo
        m_note_backup_for_redo = m_collection.remove_note(current_handle);
        if (m_note_backup_for_redo) {
            // 从管理器中注销，但保留 m_id 以便 Redo
            m_id_manager.remove_note(m_id);
        }
        // 发布事件
        editEventQueue.push({MMapEditEventType::NoteRemoved, m_id});
    }

   private:
    NoteCollection& m_collection;
    NoteIDManager& m_id_manager;
    std::unique_ptr<Note> m_note_to_add;
    std::unique_ptr<Note> m_note_backup_for_redo;
    NoteUUID m_id;  // 存储稳定 ID
};

class RemoveNoteCommand : public OperationCommand {
   public:
    // 构造函数现在接收 StableNoteID
    RemoveNoteCommand(NoteCollection& collection, NoteIDManager& id_manager,
                      NoteUUID id)
        : m_collection(collection), m_id_manager(id_manager), m_id(id) {}

    bool execute(ThreadSafeQueue<MMapEditEvent>& editEventQueue) override {
        // 通过稳定 ID 动态获取当前句柄
        NoteHandle current_handle = m_id_manager.get_handle(m_id);
        if (!current_handle.isValid()) return false;

        // 移除 Note 并备份数据
        m_removed_note_backup = m_collection.remove_note(current_handle);
        if (m_removed_note_backup) {
            // 从管理器中注销
            m_id_manager.remove_note(m_id);
            // 发布事件
            editEventQueue.push({MMapEditEventType::NoteRemoved, m_id});
            return true;
        }
        return false;
    }

    void undo(ThreadSafeQueue<MMapEditEvent>& editEventQueue) override {
        if (!m_removed_note_backup) return;

        // 1. 撤销删除就是重新添加它，会得到一个全新的句柄
        NoteHandle new_handle =
            m_collection.add_note(std::move(m_removed_note_backup));
        if (new_handle.isValid()) {
            // 通知管理器，原来的UUID现在指向了这个新句柄
            m_id_manager.update_handle(m_id, new_handle);
            // 发布事件
            editEventQueue.push({MMapEditEventType::NoteAdded, m_id});
        }
    }

   private:
    NoteCollection& m_collection;
    NoteIDManager& m_id_manager;
    NoteUUID m_id;  // 存储稳定 ID
    std::unique_ptr<Note> m_removed_note_backup;
};

/**
 * @class RemoveMultipleNotesCommand
 * @brief 一个用于原子性地删除多个音符并支持撤销的操作命令。
 */
class RemoveMultipleNotesCommand : public OperationCommand {
   public:
    /**
     * @brief 构造函数。
     * @param collection Note 数据存储的引用。
     * @param id_manager 身份管理器的引用。
     * @param ids 要删除的一组音符的稳定ID。
     */
    RemoveMultipleNotesCommand(NoteCollection& collection,
                               NoteIDManager& id_manager,
                               const std::unordered_set<NoteUUID>& ids)
        : m_collection(collection),
          m_id_manager(id_manager),
          m_ids_to_remove(ids) {}

    bool execute(ThreadSafeQueue<MMapEditEvent>& editEventQueue) override {
        // 在执行前清空旧的备份，以支持重做(redo)操作
        m_removed_notes_backup.clear();

        // 遍历所有要删除的ID
        for (const NoteUUID& id : m_ids_to_remove) {
            // 1. 通过稳定ID获取当前的临时句柄
            NoteHandle current_handle = m_id_manager.get_handle(id);
            if (!current_handle.isValid()) {
                continue;  // 如果句柄无效，跳过这个ID
            }

            // 2. 从 NoteCollection 中移除并获取其数据备份
            std::unique_ptr<Note> backup_data =
                m_collection.remove_note(current_handle);
            if (backup_data) {
                // 3. 将备份的数据与其稳定ID关联起来，存入我们的备份map中
                m_removed_notes_backup[id] = std::move(backup_data);

                // 4. 从ID管理器中注销这个ID
                m_id_manager.remove_note(id);
            }
            // 发布事件
            editEventQueue.push({MMapEditEventType::NoteRemoved, id});
        }

        // 如果我们成功删除了至少一个音符，则认为命令执行成功
        return !m_removed_notes_backup.empty();
    }

    void undo(ThreadSafeQueue<MMapEditEvent>& editEventQueue) override {
        // 如果没有备份数据，说明上次执行没有删除任何东西，直接返回
        if (m_removed_notes_backup.empty()) {
            return;
        }

        // 遍历所有备份的音符数据
        for (auto& [id, note_data] : m_removed_notes_backup) {
            if (!note_data) continue;

            // 1. 将音符数据重新添加到集合中，得到一个全新的句柄
            NoteHandle new_handle = m_collection.add_note(std::move(note_data));
            if (new_handle.isValid()) {
                // 2. 关键：修复稳定ID与新句柄之间的映射关系
                m_id_manager.update_handle(id, new_handle);
            }
            // 发布事件
            editEventQueue.push({MMapEditEventType::NoteAdded, id});
        }

        // 清空备份数据，因为所有权已经移交回 NoteCollection
        m_removed_notes_backup.clear();
    }

   private:
    NoteCollection& m_collection;
    NoteIDManager& m_id_manager;
    std::unordered_set<NoteUUID> m_ids_to_remove;  // 存储要删除的目标ID列表

    // 使用 map 来存储备份，确保每个备份数据都能准确地与其原始稳定ID对应上
    std::unordered_map<NoteUUID, std::unique_ptr<Note>> m_removed_notes_backup;
};

#endif  // MMM_OPERATIONCOMMAND_HPP
