#include <memory>
#include <mmm/map/MMap.hpp>

MMap::MMap() {}

MMap::MMap(std::string_view file) {
    basemeta.map_path = std::string(file);
    if (file.ends_with(".osu")) {
        // 读取osu
        readOsu();
    } else if (file.ends_with(".imd")) {
        readImd();
    } else if (file.ends_with(".mmm")) {
        readMMM();
    }
}

MMap::~MMap() = default;

// 访问谱面元数据
std::weak_ptr<MapMetadata> MMap::mapmeta(MapMetadataType type) {
    auto metaptr_it = metadatas.find(type);
    if (metaptr_it == metadatas.end()) {
        return std::weak_ptr<MapMetadata>();
    }
    return metaptr_it->second;
}
