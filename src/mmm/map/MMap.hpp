#ifndef MMM_MMAP_HPP
#define MMM_MMAP_HPP

#include <mmm/MetaData.hpp>
#include <mmm/NoteCollection.hpp>

// map
class MMap {
   public:
    MMap();
    virtual ~MMap();

   private:
    // (实际持有)
    // 谱面元数据集
    std::unordered_map<MapMetadataType, std::shared_ptr<MapMetadata>> metadatas;
    // 所有物件
    NoteCollection notes;
};

#endif  // MMM_MMAP_HPP
