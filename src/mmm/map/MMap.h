#ifndef M_MAP_H
#define M_MAP_H

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

 public:
  // 构造MMap
  MMap();
  // 析构MMap
  virtual ~MMap();

  // 从文件读取谱面
  void load_from_file(const char* path);
};

#endif  // M_MAP_H
