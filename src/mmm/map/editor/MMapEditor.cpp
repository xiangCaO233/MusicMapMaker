#include <memory>
#include <mmm/map/editor/MMapEditor.hpp>
#include <mmm/obj/Note.hpp>
#include <mmm/obj/rm/Composite.hpp>
#include <mmm/obj/rm/Slide.hpp>
#include <utility>

MMapEditor::MMapEditor(MMap* m, ThreadSafeQueue<MMapEditEvent>& editEventQueue)
    : map(m), operationManager(editEventQueue) {}

// 撤销和重做
void MMapEditor::undo() {
    auto cmdref = operationManager.undo();
    auto addtimingcmd = dynamic_cast<AddTimingPointCommand*>(cmdref);
    auto removetimingcmd = dynamic_cast<RemoveTimingPointCommand*>(cmdref);
    auto updatetimingcmd = dynamic_cast<UpdateTimingCommand*>(cmdref);
    if (addtimingcmd || removetimingcmd || updatetimingcmd) {
        emit timingMapUpdated();
    }
}
void MMapEditor::redo() {
    auto cmdref = operationManager.redo();
    auto addtimingcmd = dynamic_cast<AddTimingPointCommand*>(cmdref);
    auto removetimingcmd = dynamic_cast<RemoveTimingPointCommand*>(cmdref);
    auto updatetimingcmd = dynamic_cast<UpdateTimingCommand*>(cmdref);
    if (addtimingcmd || removetimingcmd || updatetimingcmd) {
        emit timingMapUpdated();
    }
}

// 创建单键音符
void MMapEditor::createNoteAt(int64_t timestamp, int track) {
    // 创建 Note 的原始数据
    auto new_note_data = std::make_unique<Note>(map);
    new_note_data->set_timestamp(timestamp);
    new_note_data->set_trackpos(track);

    // 通过操作管理器执行命令
    operationManager.executeCommand(std::make_unique<AddNoteCommand>(
        map->note_set(), map->note_uuids(), std::move(new_note_data)));
}

// 创建长键音符
void MMapEditor::createHoldAt(int64_t timestamp, int track, int64_t duration) {
    // 创建 Note 的原始数据
    auto new_note_data = std::make_unique<Hold>(map);
    new_note_data->set_notetype(NoteType::HOLD);
    new_note_data->set_timestamp(timestamp);
    new_note_data->set_trackpos(track);
    new_note_data->set_duration(duration);

    // 通过操作管理器执行命令
    operationManager.executeCommand(std::make_unique<AddNoteCommand>(
        map->note_set(), map->note_uuids(), std::move(new_note_data)));
}

// 创建滑键音符
void MMapEditor::createFlickAt(int64_t timestamp, int track,
                               int32_t delta_track) {
    // 创建 Note 的原始数据
    auto new_note_data = std::make_unique<Slide>(map);
    new_note_data->set_notetype(NoteType::SLIDE);
    new_note_data->set_timestamp(timestamp);
    new_note_data->set_trackpos(track);
    new_note_data->set_track_orbit(delta_track);

    // 通过操作管理器执行命令
    operationManager.executeCommand(std::make_unique<AddNoteCommand>(
        map->note_set(), map->note_uuids(), std::move(new_note_data)));
}

// 创建复合键音符
void MMapEditor::createCompositeWithAxis(std::list<MapAxis>& axis) {
    auto new_note_data = std::make_unique<Composite>(map);
    new_note_data->set_notetype(NoteType::COMPOSITE);
    new_note_data->set_timestamp(axis.front().time);
    new_note_data->set_trackpos(axis.front().track);
    for (auto it = axis.begin(); std::next(it) != axis.end(); ++it) {
        auto& node1 = *it;
        auto& node2 = *(std::next(it));
        auto time_same = node1.time == node2.time;
        auto track_same = node1.track == node2.track;
        std::unique_ptr<Note> child{nullptr};
        if (time_same) {
            // 创建slide物件加入组合物件
            child = std::make_unique<Slide>(map);
            auto child_slide = static_cast<Slide*>(child.get());
            child_slide->set_notetype(NoteType::SLIDE);
            child_slide->set_timestamp(node1.time);
            child_slide->set_trackpos(node1.track);
            child_slide->set_track_orbit(node2.track - node1.track);
        } else if (track_same) {
            // 创建hold物件加入组合物件
            child = std::make_unique<Hold>(map);
            auto child_hold = static_cast<Hold*>(child.get());
            child_hold->set_notetype(NoteType::HOLD);
            child_hold->set_timestamp(node1.time);
            child_hold->set_trackpos(node1.track);
            child_hold->set_duration(node2.time - node1.time);
        }

        new_note_data->add_child(std::move(child));
    }
    // 通过操作管理器执行命令
    operationManager.executeCommand(std::make_unique<AddNoteCommand>(
        map->note_set(), map->note_uuids(), std::move(new_note_data)));
}

// 删除多个物件
void MMapEditor::deleteNotes(const std::unordered_set<NoteUUID>& uuids) {
    // 通过操作管理器执行命令
    operationManager.executeCommand(
        std::make_unique<RemoveMultipleNotesCommand>(map->note_set(),
                                                     map->note_uuids(), uuids));
}

// 移动物件到指定位置
void MMapEditor::moveNote(NoteUUID uuid, int64_t timestamp, int track) {
    auto old = map->note_set().get_note(map->note_uuids().get_handle(uuid));

    std::unique_ptr<Note> new_note_data = old->clone(map);
    new_note_data->set_timestamp(timestamp);
    new_note_data->set_trackpos(track);

    updateNoteData(uuid, std::move(new_note_data));
}

// 移动多个物件到指定位置
void MMapEditor::moveNotes(
    const std::unordered_map<NoteUUID, std::pair<int64_t, int>>&
        notes_to_move) {
    if (notes_to_move.empty()) {
        return;
    }

    // 准备一个 vector 来存储所有 Note 的更新状态
    std::vector<NoteUpdateState> update_states;
    update_states.reserve(notes_to_move.size());

    // 遍历输入的 map，为每个要移动的 Note 生成新旧数据
    for (const auto& [uuid, new_pos] : notes_to_move) {
        NoteHandle handle = map->note_uuids().get_handle(uuid);
        const Note* old_note_ptr = map->note_set().get_note(handle);

        if (!old_note_ptr) {
            // 如果找不到原始Note，就跳过
            continue;
        }

        // 克隆旧数据，用于 undo
        std::unique_ptr<Note> old_data = old_note_ptr->clone(map);

        // 克隆并修改，生成新数据，用于 execute
        std::unique_ptr<Note> new_data = old_note_ptr->clone(map);
        new_data->set_timestamp(new_pos.first);  // a.k.a. timestamp
        new_data->set_trackpos(new_pos.second);  // a.k.a. track

        // 将这一对新旧状态存入 vector
        update_states.push_back(
            {uuid, std::move(old_data), std::move(new_data)});
    }

    // 如果没有任何有效的 Note 被处理，则不创建 Command
    if (update_states.empty()) {
        return;
    }

    // 创建并执行宏命令
    auto command = std::make_unique<UpdateMultipleNotesCommand>(
        map->note_set(), map->note_uuids(), std::move(update_states));

    operationManager.executeCommand(std::move(command));
}

// 拷贝到指定时间位置
void MMapEditor::copyNotesTo(const std::unordered_set<NoteUUID>& uuids_to_copy,
                             NoteUUID referenceUUID, int64_t des_time) {
    if (uuids_to_copy.empty() || referenceUUID == InvalidNoteUUID) {
        return;
    }

    // 找到参考 Note，并计算时间偏移量
    NoteHandle ref_handle = map->note_uuids().get_handle(referenceUUID);
    const Note* ref_note = map->note_set().get_note(ref_handle);
    if (!ref_note) {
        // 如果参考Note不存在，则无法计算偏移，操作失败
        return;
    }
    const int64_t time_offset = des_time - ref_note->timestamp();

    // 准备一个 vector 来存储所有新创建的 Note 的数据
    std::vector<std::unique_ptr<Note>> new_notes_data;
    new_notes_data.reserve(uuids_to_copy.size());

    // 遍历所有待拷贝的 UUID
    for (const auto& uuid : uuids_to_copy) {
        NoteHandle handle = map->note_uuids().get_handle(uuid);
        const Note* note_to_copy = map->note_set().get_note(handle);

        if (!note_to_copy) {
            continue;
        }

        // 克隆 Note 的完整数据
        std::unique_ptr<Note> new_note = note_to_copy->clone(map);

        // 应用时间偏移量，计算新的时间戳
        new_note->set_timestamp(note_to_copy->timestamp() + time_offset);

        new_notes_data.push_back(std::move(new_note));
    }

    if (new_notes_data.empty()) {
        return;
    }

    // 创建并执行批量添加命令
    auto command = std::make_unique<AddMultipleNotesCommand>(
        map->note_set(), map->note_uuids(), std::move(new_notes_data));

    operationManager.executeCommand(std::move(command));
}

// 移动物件到指定位置
void MMapEditor::updateHold(NoteUUID uuid, int64_t duration) {
    auto old = map->note_set().get_note(map->note_uuids().get_handle(uuid));

    auto new_note_data = old->clone(map);
    auto hold = static_cast<Hold*>(new_note_data.get());
    hold->set_duration(duration);

    updateNoteData(uuid, std::move(new_note_data));
}

// 更新滑键
void MMapEditor::updateSlide(NoteUUID uuid, int64_t delta_track) {
    auto old = map->note_set().get_note(map->note_uuids().get_handle(uuid));

    auto new_note_data = old->clone(map);
    auto slide = static_cast<Slide*>(new_note_data.get());
    slide->set_track_orbit(delta_track);

    updateNoteData(uuid, std::move(new_note_data));
}

// 修改音符属性
void MMapEditor::updateNoteData(NoteUUID uuid, std::unique_ptr<Note> new_data) {
    auto command = std::make_unique<UpdateNoteCommand>(
        map->note_set(), map->note_uuids(), uuid, std::move(new_data));
    operationManager.executeCommand(std::move(command));
}

// 创建timing
void MMapEditor::creatTiming(std::unique_ptr<Timing> timingData) {
    auto command = std::make_unique<AddTimingPointCommand>(
        map->timing_set(), std::move(timingData));
    operationManager.executeCommand(std::move(command));
    emit timingMapUpdated();
}

// 更新timing
void MMapEditor::updateTiming(Timing* srcTiming,
                              std::unique_ptr<Timing> newTimingData) {
    auto command = std::make_unique<UpdateTimingCommand>(
        map->timing_set(), srcTiming, std::move(newTimingData));
    operationManager.executeCommand(std::move(command));
    emit timingMapUpdated();
}

// 删除timing
void MMapEditor::deleteTiming(Timing* srcTiming) {
    auto command = std::make_unique<RemoveTimingPointCommand>(map->timing_set(),
                                                              srcTiming);
    operationManager.executeCommand(std::move(command));
    emit timingMapUpdated();
}
