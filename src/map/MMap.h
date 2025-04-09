#ifndef M_MAP_H
#define M_MAP_H

#include <memory>
#include <vector>

class HitObject;
class Timing;

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
};

#endif  // M_MAP_H
