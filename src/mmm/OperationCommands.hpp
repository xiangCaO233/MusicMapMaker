#ifndef MMM_OPERATIONCOMMAND_HPP
#define MMM_OPERATIONCOMMAND_HPP

#include <mmm/DataStructures.hpp>
#include <mmm/NoteIDManager.hpp>
#include <mmm/timing/Timing.hpp>
#include <unordered_set>

class OperationCommand {
   public:
    virtual ~OperationCommand() = default;
    virtual bool execute() = 0;
    virtual void undo() = 0;
};

class AddTimingPointCommand : public OperationCommand {
   public:
    AddTimingPointCommand(TimingMap& timing_map, const Timing& timing_to_add)
        : m_timing_map(timing_map), m_timing(timing_to_add) {}

    bool execute() override { return m_timing_map.add_timing_point(m_timing); }

    void undo() override {
        // 撤销添加就是移除
        m_timing_map.remove_timing_point(m_timing);
    }

   private:
    TimingMap& m_timing_map;
    Timing m_timing;  // 按值存储，作为数据的备份
};

class RemoveTimingPointCommand : public OperationCommand {
   public:
    RemoveTimingPointCommand(TimingMap& timing_map,
                             const Timing& timing_to_remove)
        : m_timing_map(timing_map), m_timing(timing_to_remove) {}

    bool execute() override {
        return m_timing_map.remove_timing_point(m_timing);
    }

    void undo() override {
        // 撤销移除就是重新添加
        m_timing_map.add_timing_point(m_timing);
    }

   private:
    TimingMap& m_timing_map;
    Timing m_timing;  // 按值存储，作为数据的备份
};

class RemoveTimingsAtPointCommand : public OperationCommand {
   public:
    RemoveTimingsAtPointCommand(TimingMap& timing_map, int32_t timestamp)
        : m_timing_map(timing_map), m_timestamp(timestamp) {}

    bool execute() override {
        // 调用修改后的方法，它会返回被删除的数据
        auto removed_data = m_timing_map.remove_timings_at_point(m_timestamp);

        if (removed_data.has_value()) {
            // 保存被删除的数据，以便 undo
            m_removed_timings = std::move(*removed_data);
            return true;
        }
        return false;
    }

    void undo() override {
        // 如果我们成功备份了数据
        if (!m_removed_timings.empty()) {
            // 撤销删除就是把所有备份的数据重新加回去
            for (const auto& timing : m_removed_timings) {
                m_timing_map.add_timing_point(timing);
            }
            // 清空备份，以免在 redo 后再次 undo 时出错
            m_removed_timings.clear();
        }
    }

   private:
    TimingMap& m_timing_map;
    int32_t m_timestamp;
    std::vector<Timing> m_removed_timings;  // 用于备份被删除的数据
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

    bool execute() override {
        if (!m_new_data) return false;

        // 动态地从稳定ID获取当前句柄
        NoteHandle current_handle = m_id_manager.get_handle(m_id);
        if (!current_handle.isValid()) return false;

        // 执行更新，并保存返回的“旧数据”，以便 undo
        m_old_data =
            m_collection.update_note(current_handle, std::move(m_new_data));
        return m_old_data != nullptr;
    }

    void undo() override {
        if (!m_old_data) return;

        // 同样，动态获取句柄
        NoteHandle current_handle = m_id_manager.get_handle(m_id);
        if (!current_handle.isValid()) return;

        // 撤销更新，就是用旧数据再更新一次
        m_new_data =
            m_collection.update_note(current_handle, std::move(m_old_data));
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

    bool execute() override {
        // Redo 逻辑: 如果是重做，备份数据里有 Note，稳定ID也已存在
        if (!m_note_to_add) {
            if (m_note_backup_for_redo) {
                m_note_to_add = std::move(m_note_backup_for_redo);
            } else {
                return false;  // 没有数据可以添加/重做
            }
        }

        // 执行添加，得到一个临时的 handle
        NoteHandle new_handle = m_collection.add_note(std::move(m_note_to_add));
        if (!new_handle.isValid()) return false;

        // 根据情况注册或更新 ID
        if (m_id == InvalidNoteUUID) {
            // 首次执行 (Execute): 注册一个全新的稳定 ID
            m_id = m_id_manager.register_new_note(new_handle);
        } else {
            // 重做 (Redo): 更新稳定 ID 对应的句柄
            m_id_manager.update_handle(m_id, new_handle);
        }

        return true;
    }

    void undo() override {
        // 通过稳定 ID 获取当前的句柄
        NoteHandle current_handle = m_id_manager.get_handle(m_id);
        if (!current_handle.isValid()) return;

        // 移除 Note 并备份数据，以便 Redo
        m_note_backup_for_redo = m_collection.remove_note(current_handle);
        if (m_note_backup_for_redo) {
            // 从管理器中注销，但保留 m_id 以便 Redo
            m_id_manager.remove_note(m_id);
        }
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

    bool execute() override {
        // 通过稳定 ID 动态获取当前句柄
        NoteHandle current_handle = m_id_manager.get_handle(m_id);
        if (!current_handle.isValid()) return false;

        // 移除 Note 并备份数据
        m_removed_note_backup = m_collection.remove_note(current_handle);
        if (m_removed_note_backup) {
            // 从管理器中注销
            m_id_manager.remove_note(m_id);
            return true;
        }
        return false;
    }

    void undo() override {
        if (!m_removed_note_backup) return;

        // 1. 撤销删除就是重新添加它，会得到一个全新的句柄
        NoteHandle new_handle =
            m_collection.add_note(std::move(m_removed_note_backup));
        if (new_handle.isValid()) {
            // 2. 关键：通知管理器，原来的稳定ID现在指向了这个新句柄
            m_id_manager.update_handle(m_id, new_handle);
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

    bool execute() override {
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
        }

        // 如果我们成功删除了至少一个音符，则认为命令执行成功
        return !m_removed_notes_backup.empty();
    }

    void undo() override {
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
