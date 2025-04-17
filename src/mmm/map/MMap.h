#ifndef M_MAP_H
#define M_MAP_H

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

enum class MapType {
  OSUMAP,
  RMMAP,
  MALODYMAP,
};

class HitObject;
class Timing;

// 图(谱)
class MMap {
 public:
  // 构造MMap
  MMap();
  // 析构MMap
  virtual ~MMap();

  // 图名称
  std::string map_name;

  // map文件路径
  std::filesystem::path map_file_path;

  // 音频绝对路径
  std::filesystem::path audio_file_abs_path;

  // 图类型
  MapType maptype;

  // 从文件读取谱面
  virtual void load_from_file(const char* path) = 0;
  // 全部物件
  std::vector<std::shared_ptr<HitObject>> hitobjects;
  // 全部timing
  std::vector<std::shared_ptr<Timing>> timings;

  // 查询指定位置附近的timing(优先在此之前,没有之前找之后)
  virtual void query_around_timing(
      std::vector<std::shared_ptr<Timing>>& timings, int32_t time) = 0;

  // 查询区间内有的物件
  virtual void query_object_in_range(
      std::vector<std::shared_ptr<HitObject>>& result_objects, int32_t start,
      int32_t end) = 0;
};

#endif  // M_MAP_H
