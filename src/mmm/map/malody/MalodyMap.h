#ifndef M_MALODYMAP_H
#define M_MALODYMAP_H

#include "../MMap.h"

class MalodyMap : public MMap {
   public:
    // 构造MalodyMap
    MalodyMap();
    // 通过父类构造
    MalodyMap(std::shared_ptr<MMap> srcmap);
    // 析构MalodyMap
    ~MalodyMap() override;

    // mc格式默认的元数据
    static std::shared_ptr<MapMetadata> default_metadata();

    // 从文件读取谱面
    void load_from_file(const char* path) override;

    // 写出到文件
    void write_to_file(const char* path) override;
};

#endif  // M_MALODYMAP_H
