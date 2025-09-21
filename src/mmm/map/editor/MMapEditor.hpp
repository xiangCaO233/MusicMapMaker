#ifndef MMM_MMAPEDITOR_HPP
#define MMM_MMAPEDITOR_HPP

#include <memory>
#include <mmm/OperationManager.hpp>
#include <mmm/map/MMap.hpp>
#include <tool/ToolInteractionState.hpp>
#include <unordered_set>

class MMapEditor : public QObject {
    Q_OBJECT
   public:
    MMapEditor(MMap* m, ThreadSafeQueue<MMapEditEvent>& editEventQueue);
    ~MMapEditor() = default;

    // 创建单键音符
    void createNoteAt(int64_t timestamp, int track);

    // 创建长键音符
    void createHoldAt(int64_t timestamp, int track, int64_t duration);

    // 创建滑键音符
    void createFlickAt(int64_t timestamp, int track, int32_t delta_track);

    // 创建复合键音符
    void createCompositeWithAxis(std::list<MapAxis>& axis);

    // 删除多个物件
    void deleteNotes(const std::unordered_set<NoteUUID>& uuids);

    // 移动物件到指定位置
    void moveNote(NoteUUID uuid, int64_t timestamp, int track);

    // 更新面条
    void updateHold(NoteUUID uuid, int64_t duration);

    // 更新滑键
    void updateSlide(NoteUUID uuid, int64_t delta_track);

    // 创建timing
    void creatTiming(std::unique_ptr<Timing> timingData);

    // 更新timing
    void updateTiming(Timing* srcTiming, std::unique_ptr<Timing> newTimingData);

    // 删除timing
    void deleteTiming(Timing* srcTiming);

    // 撤销和重做
    void undo();
    void redo();

   signals:
    void timingMapUpdated();

   private:
    MMap* map;
    OperationManager operationManager;

    // 修改音符属性
    void updateNoteData(NoteUUID uuid, std::unique_ptr<Note> new_data);
};

#endif  // MMM_MMAPEDITOR_HPP
