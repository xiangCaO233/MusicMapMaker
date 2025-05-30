#ifndef M_RMMAP_H
#define M_RMMAP_H

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "../MMap.h"

// 节奏大师谱面
class RMMap : public MMap {
   public:
    // 构造RMMap
    RMMap();
    // 通过父类构造
    RMMap(std::shared_ptr<MMap> srcmap);
    // 析构RMMap
    ~RMMap() override;

    // 表格行数
    int32_t table_rows{0};

    // 轨道数
    int32_t max_orbits{0};

    // 版本--一般为ez,nm,hd,mx之类的
    std::string Version;

    // 插入物件
    void insert_hitobject(std::shared_ptr<HitObject> hitobject) override;

    // 移除物件
    void remove_hitobject(std::shared_ptr<HitObject> hitobject) override;

    // 从文件读取谱面
    void load_from_file(const char* path) override;

    // 写出到文件
    void write_to_file(const char* path) override;

    // 写出一个物件
    void write_note(std::ofstream& os, const std::shared_ptr<Note>& note);

    // 写出文件是否合法
    bool is_write_file_legal(const char* file, std::string& res) override;

    // imd格式默认的元数据
    static std::shared_ptr<MapMetadata> default_metadata();
};

// 二进制读取器
class BinaryReader {
   public:
    // 构造ImdReader
    BinaryReader();

    // 析构ImdReader
    virtual ~BinaryReader();

    // 读取指定指针位置的数据
    template <typename T>
    T read_value(const char* data, bool is_little_endian = true) {
        T value;
        std::memcpy(&value, data, sizeof(T));

        if (!is_little_endian) {
            char* ptr = reinterpret_cast<char*>(&value);
            std::reverse(ptr, ptr + sizeof(T));
        }

        return value;
    }
};

#endif  // M_RMMAP_H
