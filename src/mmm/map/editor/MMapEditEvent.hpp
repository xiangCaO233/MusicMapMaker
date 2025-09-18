#ifndef MMM_MMAPEDITEVENT_HPP
#define MMM_MMAPEDITEVENT_HPP

#include <mmm/ObjectHandle.hpp>
#include <variant>

class Timing;

enum class MMapEditEventType {
    // 物件添加
    NoteAdded,
    // 物件移除
    NoteRemoved,
    // 物件更新
    NoteUpdated,
    // timing添加
    TimingAdded,
    // timing移除
    TimingRemoved,
    // timing更新
    TimingUpdated,
};

using MapEditData = std::variant<NoteUUID, Timing*>;

// 编辑事件
struct MMapEditEvent {
    MMapEditEventType type;
    MapEditData editData;
};

#endif  // MMM_MMAPEDITEVENT_HPP
