#ifndef M_RMMAP_H
#define M_RMMAP_H

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "../MMap.h"

// 节奏大师谱面
class RMMap : public MMap {
   public:
    // 构造RMMap
    RMMap();
    // 析构RMMap
    ~RMMap() override;

    // 表格行数
    int32_t table_rows{0};

    // 轨道数
    int32_t max_orbits{0};

    // 版本--一般为ez,nm,hd,mx之类的
    std::string version;

    // 从文件读取谱面
    void load_from_file(const char* path) override;

   protected:
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
