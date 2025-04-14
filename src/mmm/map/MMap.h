#ifndef M_MAP_H
#define M_MAP_H

#include <filesystem>
#include <memory>
#include <vector>

class HitObject;
class Timing;

// 图(谱)
class MMap {
  // 全部物件
  std::vector<std::shared_ptr<HitObject>> hitobjects;
  // 全部timing
  std::vector<std::shared_ptr<Timing>> timings;

  void load_imd(std::filesystem::path& imd_path);

 public:
  // 构造MMap
  MMap();
  // 析构MMap
  virtual ~MMap();

  // 从文件读取谱面
  virtual void load_from_file(const char* path) = 0;
};

#endif  // M_MAP_H
