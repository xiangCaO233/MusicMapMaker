#include <mmm/map/MMapEditor.hpp>
#include <mmm/obj/Note.hpp>

MMapEditor::MMapEditor(MMap* m) : map(m) {}

// 撤销和重做
void MMapEditor::undo() { operationManager.undo(); }
void MMapEditor::redo() { operationManager.redo(); }

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

// 删除多个物件
void MMapEditor::deleteNotes(const std::unordered_set<NoteUUID>& note_uuids) {
    // 通过操作管理器执行命令
    operationManager.executeCommand(
        std::make_unique<RemoveMultipleNotesCommand>(
            map->note_set(), map->note_uuids(), note_uuids));
}

// 修改音符属性
void MMapEditor::updateNoteData(NoteUUID id_to_update,
                                std::unique_ptr<Note> new_data) {
    auto command = std::make_unique<UpdateNoteCommand>(
        map->note_set(), map->note_uuids(), id_to_update, std::move(new_data));
    operationManager.executeCommand(std::move(command));
}
