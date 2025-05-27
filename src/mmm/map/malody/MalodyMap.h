#ifndef M_MALODYMAP_H
#define M_MALODYMAP_H

#include <cstdint>

#include "../MMap.h"

class MalodyMap : public MMap {
   public:
    // 构造MalodyMap
    MalodyMap();
    // 通过父类构造
    MalodyMap(std::shared_ptr<MMap> srcmap);
    // 析构MalodyMap
    ~MalodyMap() override;

    // malody特有的属性
    int32_t mapid;
    // 0为key模式-7为slide模式
    int32_t mapmode;

    // 图片相对路径
    std::string background_rpath;
    std::string cover_rpath;

    // mc格式默认的元数据
    static std::shared_ptr<MapMetadata> default_metadata();

    // 从文件读取谱面
    void load_from_file(const char* path) override;

    // 写出到文件
    void write_to_file(const char* path) override;
};

#endif  // M_MALODYMAP_H
