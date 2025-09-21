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
