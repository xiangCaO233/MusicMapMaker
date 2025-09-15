#ifndef MMM_MMAPEDITOR_HPP
#define MMM_MMAPEDITOR_HPP

#include <mmm/OperationManager.hpp>
#include <mmm/map/MMap.hpp>
#include <unordered_set>

class MMapEditor {
   public:
    MMapEditor(MMap* m);
    ~MMapEditor() = default;

    // 创建单键音符
    void createNoteAt(int64_t timestamp, int track);

    // 创建长键音符
    void createHoldAt(int64_t timestamp, int track, int64_t duration);

    // 删除多个物件
    void deleteNotes(const std::unordered_set<NoteUUID>& note_uuids);

    // 修改音符属性
    void updateNoteData(NoteUUID id_to_update, std::unique_ptr<Note> new_data);

    // 撤销和重做
    void undo();
    void redo();

   private:
    MMap* map;
    OperationManager operationManager;
};

#endif  // MMM_MMAPEDITOR_HPP
