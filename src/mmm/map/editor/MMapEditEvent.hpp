#ifndef MMM_MMAPEDITEVENT_HPP
#define MMM_MMAPEDITEVENT_HPP

#include <list>
#include <mmm/ObjectHandle.hpp>
#include <variant>

class Timing;

enum class MMapEditEventType {
    // 物件添加
    NoteAdded,
    NotesAdded,
    // 物件移除
    NoteRemoved,
    NotesRemoved,
    // 物件更新
    NoteUpdated,
    NotesUpdated,
    // timing添加
    TimingAdded,
    // timing移除
    TimingRemoved,
    // timing更新
    TimingUpdated,
};

using NoteUUIDS = std::list<NoteUUID>;
using MapEditData = std::variant<NoteUUID, NoteUUIDS, Timing*>;

// 编辑事件
struct MMapEditEvent {
    MMapEditEventType type;
    MapEditData editData;
};

#endif  // MMM_MMAPEDITEVENT_HPP
