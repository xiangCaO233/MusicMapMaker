#include <mmm/map/MMap.hpp>

MMap::MMap() {}

MMap::MMap(std::string_view file) {
    map_path = std::string(file);
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
