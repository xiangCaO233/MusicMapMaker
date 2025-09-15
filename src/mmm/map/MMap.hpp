#ifndef MMM_MMAP_HPP
#define MMM_MMAP_HPP

#include <ice/manage/AudioTrack.hpp>
#include <memory>
#include <mmm/DataStructures.hpp>
#include <mmm/MetaData.hpp>
#include <mmm/NoteIDManager.hpp>
#include <mmm/map/BaseMapMeta.hpp>
#include <mmm/timing/Beat.hpp>
#include <unordered_map>

class MProject;
class MMapEditor;

// map
class MMap {
   public:
    MMap();
    explicit MMap(std::string_view file);

    virtual ~MMap();

    // 绑定项目
    void bind_project(MProject* prj) { project_ref = prj; }

    // 获取项目引用
    MProject* project() { return project_ref; }

    // 直接访问物件集合
    NoteCollection& note_set() { return notes; }
    NoteIDManager& note_uuids() { return noteUUIDManager; }

    // 直接访问时间点集合
    TimingMap& timing_set() { return timings; }

    // 直接访问拍时间线
    BeatTimeline& beat_timeline() { return beatTimeline; }

    // 直接访问拍信息
    BeatInfo& beat_info() { return beatInfo; }

    // 访问谱面元数据
    std::weak_ptr<MapMetadata> map_metadata(MapMetadataType type);

    // 访问基本元数据
    BaseMapMeta& base_metadata() { return basemeta; };

    // 访问对应的谱面编辑器
    MMapEditor& editor() { return *mapeditor; }

    // 设置主音轨
    void set_maintrack(const std::shared_ptr<ice::AudioTrack>& track);

   private:
    // (实际持有)
    // 谱面元数据集
    std::unordered_map<MapMetadataType, std::shared_ptr<MapMetadata>> metadatas;

    // 所有物件
    NoteCollection notes;
    // 物件uuid管理器
    NoteIDManager noteUUIDManager;

    // 所有时间点
    TimingMap timings;

    // 所有有效拍信息
    BeatInfo beatInfo;
    BeatTimeline beatTimeline;

    // 基础谱面信息
    BaseMapMeta basemeta;

    // 一对一编辑器
    std::unique_ptr<MMapEditor> mapeditor;

    // 项目引用
    MProject* project_ref;

    // 谱面io操作
    void readOsu();
    void writeOsu() {};

    void readImd() {};
    void writeImd() {};

    void readMc() {};
    void writeMc() {};

    void readMMM() {};
    void writeMMM() {};

    // 更新拍信息(智能识别分拍)
    void analyzeBeatInfo();
};

#endif  // MMM_MMAP_HPP
