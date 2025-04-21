#ifndef M_MAP_H
#define M_MAP_H

#include <QRectF>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "../Beat.h"
#include "../hitobject/HitObject.h"
#include "../hitobject/Note/Hold.h"
#include "../timing/Timing.h"

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

  // 背景图-如果有
  std::filesystem::path bg_path;

  // 谱面时长--计算
  int32_t map_length = 0;

  // 物件比较器
  struct HitObjectComparator {
    bool operator()(const std::shared_ptr<HitObject>& a,
                    const std::shared_ptr<HitObject>& b) const {
      return a->timestamp < b->timestamp;
    }
  };

  // 全部物件
  std::multiset<std::shared_ptr<HitObject>, HitObjectComparator> hitobjects;

  // 用于识别重叠时间域的长条物件缓存表
  std::multiset<std::shared_ptr<Hold>> temp_hold_list;

  // timing比较器
  struct TimingComparator {
    bool operator()(const std::shared_ptr<Timing>& a,
                    const std::shared_ptr<Timing>& b) const {
      return a->timestamp < b->timestamp;
    }
  };

  // 全部timing
  std::multiset<std::shared_ptr<Timing>, TimingComparator> timings;

  // 用于识别重叠时间的timing列表缓存map
  std::unordered_map<int32_t, std::vector<std::shared_ptr<Timing>>>
      temp_timing_map;

  // beat比较器
  struct BeatComparator {
    bool operator()(const std::shared_ptr<Beat>& a,
                    const std::shared_ptr<Beat>& b) const {
      return a->start_timestamp < b->start_timestamp;
    }
  };

  // 全部拍-自动分析分拍和bpm,变速
  std::multiset<std::shared_ptr<Beat>, BeatComparator> beats;

  // 从文件读取谱面
  virtual void load_from_file(const char* path) = 0;

  // 生成此拍的分拍策略
  void generate_divisor_policy(std::shared_ptr<Beat>& beat);

  // 擦除指定范围内的拍
  void erase_beats(double start, double end);

  // 有序的添加timing-会分析并更新拍
  virtual void insert_timing(std::shared_ptr<Timing> timing);

  // 移除timing-会分析并更新拍
  virtual void remove_timing(std::shared_ptr<Timing> timing);

  // 查询指定位置附近的timing(优先在此之前,没有之前找之后)
  virtual void query_around_timing(
      std::vector<std::shared_ptr<Timing>>& timings, int32_t time);

  // 查询区间窗口内的timing
  virtual void query_timing_in_range(
      std::vector<std::shared_ptr<Timing>>& result_timings, int32_t start,
      int32_t end);

  // 查询区间窗口内的拍
  virtual void query_beat_in_range(
      std::vector<std::shared_ptr<Beat>>& result_beats, int32_t start,
      int32_t end);

  // 查询区间内有的物件
  virtual void query_object_in_range(
      std::vector<std::shared_ptr<HitObject>>& result_objects, int32_t start,
      int32_t end, bool with_longhold = false);
};

#endif  // M_MAP_H
