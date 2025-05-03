#include "MMap.h"

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <vector>

#include "../../util/mutil.h"
#include "../hitobject/HitObject.h"
#include "../hitobject/Note/Note.h"
#include "../hitobject/Note/rm/ComplexNote.h"
#include "../timing/Timing.h"

MMap::MMap() {
  // 初始化播放回调对象
  audio_pos_callback = std::make_shared<AudioEnginPlayCallback>();
}

MMap::~MMap() = default;

// 生成此拍的分拍策略
void MMap::generate_divisor_policy(const std::shared_ptr<Beat>& beat) {
  // 计算这拍之内的可能分拍策略
  // 获取此拍范围内的物件
  std::vector<std::shared_ptr<HitObject>> notes;
  query_object_in_range(notes, beat->start_timestamp, beat->end_timestamp);

  if (notes.empty()) {
    // 没有物件,默认1分
    beat->divisors = 1;
  } else {
    std::multiset<std::shared_ptr<HitObject>, HitObjectComparator> objs;
    for (const auto& note : notes) {
      // 设置拍引用
      note->beatinfo = beat;
      // set自动排序
      objs.insert(note);
    }

    // 允许5ms误差
    beat->divisors = mutil::calculateDivisionStrategy(objs, beat, 5);
  }
}

// 擦除指定范围内的拍
void MMap::erase_beats(double start, double end) {
  if (beats.empty()) return;

  // 找到此时间区间内的第一拍位置
  auto beat_lower_bound = std::make_shared<Beat>(200, start, start);
  // 找到第一个大于或等于 lower_bound 的元素
  auto beat_startit = beats.lower_bound(beat_lower_bound);

  // 找到此时间区间内的最后一拍位置
  auto beat_upper_bound = std::make_shared<Beat>(200, end, end);
  // 找到第一个严格大于 upper_bound 的元素
  auto beat_endit = beats.upper_bound(beat_upper_bound);
  // 移除此区间全部的beat
  // 确保 beat_endit 不是 begin()，否则 -- 会越界
  if (beat_endit != beats.begin()) {
    beat_endit--;  // 现在指向最后一个 ≤ end 的 Beat
  } else {
    // 所有 Beat 都 > end，区间内无 Beat 可删
    return;
  }
  auto start_legal = beat_startit != beats.end();
  if (start_legal) {
    auto start_less_than_end =
        *beat_startit != *beat_endit && **beat_startit <= **beat_endit;
    // 确保 beat_startit ≤ beat_endit（否则区间无效）
    if (start_less_than_end) {
      // 删除 [beat_startit, beat_endit] 区间内的所有 Beat
      beats.erase(beat_startit, ++beat_endit);  // erase 是 [first, last)
    }
  }
}

// 有序的添加timing-会分析并更新拍
void MMap::insert_timing(const std::shared_ptr<Timing>& timing) {
  // 加入timing列表
  auto insertit = timings.insert(timing);
  ++insertit;

  auto temp_timing_list_it = temp_timing_map.find(timing->timestamp);
  if (temp_timing_list_it == temp_timing_map.end()) {
    // 添加映射
    temp_timing_list_it = temp_timing_map.try_emplace(timing->timestamp).first;
  }
  temp_timing_list_it->second.emplace_back(timing);

  // 更新此timing时间开始到下一基准timing之前的拍/之后没有就一直到maplegnth
  std::shared_ptr<Timing> next_base_timing = nullptr;
  while (insertit != timings.end()) {
    if ((*insertit)->is_base_timing) {
      next_base_timing = *insertit;
      break;
    }
    ++insertit;
  }

  // 区分变速timing和基准timing
  if (timing->is_base_timing) {
    // 是基准timing--音乐bpm变化
    // 界定处理时间范围
    auto start = timing->timestamp;
    uint32_t end;

    // 区分有无
    if (next_base_timing) {
      end = next_base_timing->timestamp;
    } else {
      end = map_length;
    }

    // 擦除范围内的拍
    erase_beats(start, end);

    // 当前处理到的时间
    auto current_process_time = double(start);
    // 每一拍的时间
    double beattime = 60.0 / timing->basebpm * 1000.0;

    // 获取此时间范围内的timing,可能有变速timing
    std::vector<std::shared_ptr<Timing>> src_timings;
    query_timing_in_range(src_timings, start, end);

    if (!src_timings.empty()) {
      // 范围内有其他timing
      auto it = src_timings.begin();
      // 这里都是变速timing了
      while (current_process_time < end) {
        // 每一拍
        auto beat = std::make_shared<Beat>(timing->bpm, current_process_time,
                                           current_process_time + beattime);
        if (beat->start_timestamp >= (*it)->timestamp) {
          if (it + 1 != src_timings.end()) {
            // 区间内当前变速timing后面还有变速timing
            if (beat->start_timestamp < (*(it + 1))->timestamp) {
              // 此拍在此变速timing后在下一变速timing前-使用此timing的时间线缩放
              beat->timeline_zoom = (*it)->bpm;
            } else {
              // 到了下一个变速timing后
              // 更新一下当前使用的变速timing重来
              it++;
              continue;
            }
          } else {
            // 之后就这一个变速timing了--使用此timing的时间线缩放
            beat->timeline_zoom = (*it)->bpm;
          }
        } else {
          // 还没到变速timing位置,不使用时间线缩放--直接将此timing作为结果
        }

        // 计算这拍之内的可能分拍策略
        generate_divisor_policy(beat);
        if (beat->divisors != 1) {
          // 锁定分拍策略
          beat->divisors_customed = true;
        }
        // 插入beat结果
        beats.insert(beat);
        current_process_time += beattime;
      }
    } else {
      while (current_process_time < end) {
        // 每一拍
        auto beat =
            std::make_shared<Beat>(timing->basebpm, current_process_time,
                                   current_process_time + beattime);
        // 计算这拍之内的可能分拍策略
        generate_divisor_policy(beat);
        if (beat->divisors != 1) {
          // 锁定分拍策略
          beat->divisors_customed = true;
        }
        // 插入beat结果
        beats.insert(beat);
        current_process_time += beattime;
      }
    }
  } else {
    // 变速timing-音乐bpm不变/视觉变速(仅时间线缩放变化)
    // 获取此timing的下一timing(无论是基准还是变速)
    std::shared_ptr<Timing> next_timing = nullptr;
    if (insertit != timings.end()) {
      // 有下一个timing
      next_timing = *insertit;
    } else {
      // 没有,不管
    }
    const double start = timing->timestamp;
    double end;

    // 检查是否有
    if (next_timing) {
      // 区间为timing到下一timing之间的拍
      end = next_timing->timestamp;
    } else {
      // 没有,区间到结尾
      end = map_length;
    }

    // 生成新拍
    // 修改范围内的拍的时间线缩放
    // 当前处理到的时间
    auto beatdummy = std::make_shared<Beat>(double(start));
    auto current_process_beatit = beats.lower_bound(beatdummy);

    // 每一拍的实际时间
    double beattime = 60.0 / timing->basebpm * 1000.0;

    while ((*current_process_beatit)->end_timestamp < end) {
      // 修改时间线缩放
      (*current_process_beatit)->timeline_zoom = timing->bpm;
      ++current_process_beatit;
    }
  }
}

// 移除timing-会分析并更新拍
void MMap::remove_timing(std::shared_ptr<Timing> timing) {}

// 查询指定位置附近的timing列表
// 返回3个timing-基准timing,前一个基准timing,前一个变速timing和后一个变速timing
// 包含传入时间点时返回4个或5个(放在[3]和[4]),即此时间点同时存在基准和变速timing
void MMap::query_around_timing(
    std::vector<std::shared_ptr<Timing>>& result_timings, int32_t time) {
  if (timings.empty()) return;
  // 查找到第一个大于等于此时间的timing迭代器
  auto it = std::upper_bound(
      timings.begin(), timings.end(), time,
      [](int time, const auto& timing) { return timing->timestamp >= time; });

  // 确定使用的timing
  if (it == timings.end()) {
    // 没找到比当前时间靠后的timing
    // 使用最后一个timing
    it = timings.end();
    --it;
  } else {
    // 找到了比当前时间靠后的timing
    if (it == timings.begin()) {
      // 找到的是第一个
      // 就使用这个timing
    } else {
      // 使用前一个
      --it;
    }
  }
  std::shared_ptr<Timing> res = *it;

  // 查找重复时间点的timing表
  auto timing_list_it = temp_timing_map.find(res->timestamp);

  // 添加到传入引用
  for (const auto& timing : timing_list_it->second) {
    result_timings.emplace_back(timing);
  }
}

// 查询区间窗口内的timing
void MMap::query_timing_in_range(
    std::vector<std::shared_ptr<Timing>>& result_timings, int32_t start,
    int32_t end) {
  // TODO(xiang 2025-05-03): 实现在指定区间内查询timing
}

// 查询时间区间窗口内的拍
void MMap::query_beat_in_range(std::vector<std::shared_ptr<Beat>>& result_beats,
                               int32_t start, const int32_t end) {
  result_beats.clear();
  auto lower_bound = std::make_shared<Beat>(200, start, start);

  // 找到第一个大于或等于 lower_bound 的元素
  auto it = beats.lower_bound(lower_bound);

  // 遍历直到超出 upper_bound
  while (it != beats.end() && (*it)->end_timestamp < end) {
    result_beats.push_back(*it);
    ++it;
  }
}

struct SharedPtrCompare {
  bool operator()(const std::shared_ptr<HitObject>& a,
                  const std::shared_ptr<HitObject>& b) const {
    // 基类有虚函数 lessThan
    return a->lessThan(b.get());
  }
};

// 查询区间窗口内有的物件
void MMap::query_object_in_range(
    std::vector<std::shared_ptr<HitObject>>& result_objects, int32_t start,
    const int32_t end, bool with_longhold) {
  // 全部物件
  thread_local std::multiset<std::shared_ptr<HitObject>, HitObjectComparator>
      adding_objects;

  // 自动去重+排序
  // std::set<std::shared_ptr<HitObject>, SharedPtrCompare> adding_objects;
  /*
   *lower_bound(key)	返回第一个 ≥ key 的元素的迭代器。
   *upper_bound(key)	返回第一个 > key 的元素的迭代器。
   */
  if (with_longhold) {
    // 带超级长条
    // 面尾时间>窗口起始时间且面头<结束时间就加入列表
    for (const auto& hold : temp_hold_list) {
      if (hold->timestamp < end && hold->timestamp + hold->hold_time > start) {
        adding_objects.insert(hold);
      }
    }
  }

  // 窗口内的物件
  auto startit = hitobjects.lower_bound(std::make_shared<Note>(start, 0));
  int count = 0;
  int deviation = 0;
  while (startit != hitobjects.end() && startit != hitobjects.begin() &&
         deviation < 5) {
    --startit;
    ++count;
    deviation = std::abs((*startit)->timestamp - start);
    if (deviation < 5) {
      adding_objects.insert(*startit);
    }
  }

  // 返回
  while (count > 0) {
    ++startit;
    --count;
  }

  ComplexNote* temp_comp = nullptr;

  for (; startit != hitobjects.end() && (*startit)->timestamp < end;
       ++startit) {
    // 添加组合键
    auto note = std::dynamic_pointer_cast<Note>(*startit);
    if (note && note->parent_reference && temp_comp != note->parent_reference) {
      temp_comp = note->parent_reference;
      // 将所有子物件加入渲染队列
      for (const auto& subnote : temp_comp->child_notes) {
        adding_objects.insert(subnote);
      }
    }
    // 添加窗口内物件
    adding_objects.insert(*startit);
  }

  // 按set遍历加入结果
  for (const auto& obj : adding_objects) result_objects.emplace_back(obj);
  adding_objects.clear();
}
