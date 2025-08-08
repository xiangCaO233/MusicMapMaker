#ifndef MMM_MMAP_HPP
#define MMM_MMAP_HPP

#include <filesystem>
#include <mmm/MetaData.hpp>
#include <mmm/NoteCollection.hpp>

// map
class MMap {
   public:
    MMap();
    explicit MMap(std::string_view file);

    virtual ~MMap();

    // 直接访问物件集合
    NoteCollection& note_set() { return notes; }

   private:
    // (实际持有)
    // 谱面元数据集
    std::unordered_map<MapMetadataType, std::shared_ptr<MapMetadata>> metadatas;

    // 所有物件
    NoteCollection notes;

    // 谱面文件路径
    std::filesystem::path map_path;

    // 主音频文件路径
    std::filesystem::path main_audio_path;

    void readOsu();
    void writeOsu();

    void readImd();
    void writeImd();

    void readMc();
    void writeMc();
};

#endif  // MMM_MMAP_HPP
