#include "MalodyMap.h"

// 构造MalodyMap
MalodyMap::MalodyMap() {}

// 通过父类构造
MalodyMap::MalodyMap(std::shared_ptr<MMap> srcmap) {}

// 析构MalodyMap
MalodyMap::~MalodyMap() = default;

// mc格式默认的元数据
std::shared_ptr<MapMetadata> MalodyMap::default_metadata() {}

// 从文件读取谱面
void MalodyMap::load_from_file(const char* path) {}

// 写出到文件
void MalodyMap::write_to_file(const char* path) {}
