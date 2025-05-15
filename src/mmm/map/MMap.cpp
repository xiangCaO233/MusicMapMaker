#include "MMap.h"

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include "../../util/mutil.h"
#include "../hitobject/HitObject.h"
#include "../hitobject/Note/HoldEnd.h"
#include "../hitobject/Note/Note.h"
#include "../hitobject/Note/rm/ComplexNote.h"
#include "../hitobject/Note/rm/Slide.h"
#include "../timing/Timing.h"
#include "colorful-log.h"
#include "osu/OsuMap.h"
#include "rm/RMMap.h"

MMap::MMap() {
    // 初始化播放回调对象
    audio_pos_callback = std::make_shared<AudioEnginPlayCallback>();
}

MMap::~MMap() = default;

// 从文件读取谱面
void MMap::load_from_file(const char* path) {}

// 注册元数据
void MMap::register_metadata(MapMetadataType type) {
    switch (type) {
        case MapMetadataType::IMD: {
            metadatas[type] = RMMap::default_metadata();
            break;
        }
        case MapMetadataType::OSU: {
            metadatas[type] = OsuMap::default_metadata();
            break;
        }
        default:
            break;
    }
}

// 插入物件
void MMap::insert_hitobject(std::shared_ptr<HitObject> hitobject) {
    // 普通物件
    auto note = std::dynamic_pointer_cast<Note>(hitobject);
    if (note) {
        switch (note->note_type) {
            case NoteType::NOTE: {
                hitobjects.insert(note);
                break;
            }
            case NoteType::SLIDE: {
                // 滑键
                // 放入主区,并在主区添加面尾物件
                auto insert_slide = std::static_pointer_cast<Slide>(note);
                hitobjects.insert(insert_slide);
                hitobjects.insert(insert_slide->slide_end_reference);
                break;
            }
            case NoteType::HOLD: {
                // 面条
                auto insert_hold = std::static_pointer_cast<Hold>(note);
                // 放入主区,缓冲区,并在主区添加面尾物件
                hitobjects.insert(insert_hold);
                temp_hold_list.insert(insert_hold);
                hitobjects.insert(insert_hold->hold_end_reference);
                break;
            }
            case NoteType::COMPLEX: {
                // 组合键
                auto insert_complex =
                    std::static_pointer_cast<ComplexNote>(note);
                for (const auto& child_note : insert_complex->child_notes) {
                    // 递归插入所有子物件
                    insert_hitobject(child_note);
                }
                // 然后添加自身
                hitobjects.insert(insert_complex);

                break;
            }
        }
    } else {
        // 非note的其他物件
        hitobjects.insert(hitobject);
    }
}

// 移除物件
void MMap::remove_hitobject(std::shared_ptr<HitObject> hitobject) {
    auto note = std::dynamic_pointer_cast<Note>(hitobject);
    if (note) {
        // 添加相同物件的迭代器到数组
        auto insert_same_objs =
            [&](std::vector<decltype(hitobjects.begin())>& to_erase,
                std::shared_ptr<HitObject> obj) {
                auto it = hitobjects.lower_bound(obj);
                // 前移到上一时间戳
                while (it != hitobjects.begin() &&
                       note->timestamp - it->get()->timestamp < 10) {
                    --it;
                }

                // 添加滑头的即将删除迭代器
                while (it != hitobjects.end() &&
                       (it->get()->timestamp - note->timestamp) < 5) {
                    if (it->get()->equals(note)) {
                        to_erase.push_back(it);
                    }
                    ++it;
                }
            };
        // 添加相同物件的迭代器到数组
        auto insert_same_tempobjs =
            [&](std::vector<decltype(temp_hold_list.begin())>& to_erase,
                std::shared_ptr<Hold> obj) {
                auto it = temp_hold_list.lower_bound(obj);
                // 前移到上一时间戳
                while (it != temp_hold_list.begin() &&
                       note->timestamp - it->get()->timestamp < 10) {
                    --it;
                }

                // 添加滑头的即将删除迭代器
                while (it != temp_hold_list.end() &&
                       (it->get()->timestamp - note->timestamp) < 5) {
                    if (it->get()->equals(note)) {
                        to_erase.push_back(it);
                    }
                    ++it;
                }
            };
        // 从主集合移除
        auto remove_from_main =
            [&](std::vector<decltype(hitobjects.begin())>& to_erase) {
                // 从后往前删除，避免迭代器失效
                for (auto rit = to_erase.rbegin(); rit != to_erase.rend();
                     ++rit) {
                    XWARN(QString("移除map源物件:t[%1]")
                              .arg((*rit)->get()->timestamp)
                              .toStdString());
                    hitobjects.erase(*rit);
                }
            };
        // 从缓存面条集合移除
        auto remove_from_bufferholds =
            [&](std::vector<decltype(temp_hold_list.begin())>& to_erase) {
                // 从后往前删除，避免迭代器失效
                for (auto rit = to_erase.rbegin(); rit != to_erase.rend();
                     ++rit) {
                    XWARN(QString("移除map源物件:t[%1]")
                              .arg((*rit)->get()->timestamp)
                              .toStdString());
                    temp_hold_list.erase(*rit);
                }
            };

        switch (note->note_type) {
            case NoteType::NOTE: {
                // 删除单键-可能的重叠物件一起删
                auto insert_slide = std::static_pointer_cast<Slide>(note);
                // 存储要删除的迭代器（可能有多个匹配项）
                std::vector<decltype(hitobjects.begin())> to_erase;
                insert_same_objs(to_erase, note);
                remove_from_main(to_erase);

                break;
            }
            case NoteType::SLIDE: {
                // 滑键
                // 从主区移除,并在主区移除对应面尾物件
                auto insert_slide = std::static_pointer_cast<Slide>(note);

                // 存储要删除的迭代器（可能有多个匹配项）
                std::vector<decltype(hitobjects.begin())> to_erase;

                // 添加滑头和滑尾
                insert_same_objs(to_erase, insert_slide);
                // 从主集合移除物件
                remove_from_main(to_erase);

                to_erase.clear();
                insert_same_objs(to_erase, insert_slide->slide_end_reference);
                // 从主集合移除物件
                remove_from_main(to_erase);
                break;
            }
            case NoteType::HOLD: {
                // 面条
                auto remove_hold = std::static_pointer_cast<Hold>(note);
                // 从主区和缓冲区移除,并在主区移除面尾物件
                // 存储要删除的迭代器（可能有多个匹配项）
                std::vector<decltype(hitobjects.begin())> to_erase;

                // 添加面头和面尾
                insert_same_objs(to_erase, remove_hold);
                // 从主集合移除物件
                remove_from_main(to_erase);

                // 防止迭代器失效
                to_erase.clear();
                insert_same_objs(to_erase, remove_hold->hold_end_reference);
                // 从主集合移除物件
                remove_from_main(to_erase);

                // 面条缓存
                std::vector<decltype(temp_hold_list.begin())> to_erase_temp;
                insert_same_tempobjs(to_erase_temp, remove_hold);
                remove_from_bufferholds(to_erase_temp);

                break;
            }
            case NoteType::COMPLEX: {
                // 组合键
                auto remove_complex =
                    std::static_pointer_cast<ComplexNote>(note);
                for (const auto& child_note : remove_complex->child_notes) {
                    // 递归移除所有子物件
                    remove_hitobject(child_note);
                }
                // 然后移除自身
                // 存储要删除的迭代器（可能有多个匹配项）
                std::vector<decltype(hitobjects.begin())> to_erase;
                hitobjects.erase(remove_complex);
                break;
            }
        }
    } else {
        hitobjects.erase(hitobject);
    }
}

// 执行操作
void MMap::execute_edit_operation(ObjEditOperation& operation) {
    // TODO(xiang 2025-05-08): 实现执行操作

    bool remove_parent{false};
    // 移除src的物件
    if (!operation.src_objects.empty())
        for (const auto& srcobj : operation.src_objects) {
            remove_hitobject(srcobj);
        }

    // 插入des的物件
    if (!operation.des_objects.empty()) {
        for (const auto& obj : operation.des_objects) {
            // 添加处若有完全相同的物件-取消此次添加
            bool add{true};
            auto it = hitobjects.lower_bound(obj);
            // 前移到上一时间戳
            while (it != hitobjects.begin() &&
                   obj->timestamp - it->get()->timestamp < 10)
                --it;
            while (it != hitobjects.end() &&
                   (it->get()->timestamp - obj->timestamp) < 5) {
                if (obj->equals(*it)) {
                    add = false;
                    break;
                }
                ++it;
            }
            if (add) {
                insert_hitobject(obj);
                XWARN(QString("添加map目标物件:t[%1]")
                          .arg(obj->timestamp)
                          .toStdString());
            } else {
                XWARN(QString("t[%1]已有物件,跳过添加")
                          .arg(obj->timestamp)
                          .toStdString());
            }
        }
    }
}

void MMap::execute_edit_operation(TimingEditOperation& operation) {}

// 生成此拍的分拍策略
void MMap::generate_divisor_policy(const std::shared_ptr<Beat>& beat) {
    // 计算这拍之内的可能分拍策略
    // 获取此拍范围内的物件
    std::multiset<std::shared_ptr<HitObject>, HitObjectComparator> notes;
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
        temp_timing_list_it =
            temp_timing_map.try_emplace(timing->timestamp).first;
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
                auto beat =
                    std::make_shared<Beat>(timing->bpm, current_process_time,
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
                auto beat = std::make_shared<Beat>(
                    timing->basebpm, current_process_time,
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
    result_timings.clear();
    auto lower_bound = std::make_shared<Timing>();
    lower_bound->timestamp = start;

    // 找到第一个大于或等于 lower_bound 的元素
    auto it = timings.lower_bound(lower_bound);

    // 遍历直到超出 upper_bound
    while (it != timings.end() && (*it)->timestamp < end) {
        result_timings.push_back(*it);
        ++it;
    }
}

// 查询区间窗口内的timings
void MMap::query_timing_in_range(
    std::vector<std::vector<std::shared_ptr<Timing>>*>& result_timingss,
    int32_t start, int32_t end) {
    result_timingss.clear();

    // 找到第一个大于或等于 lower_bound 的元素
    auto it = temp_timing_map.lower_bound(start);

    // 遍历直到超出 upper_bound
    while (it != temp_timing_map.end() && it->first < end) {
        result_timingss.push_back(&it->second);
        ++it;
    }
}

// 查询指定时间之前的拍
std::list<std::shared_ptr<Beat>> MMap::query_beat_before_time(int32_t time) {
    if (beats.empty()) return {};

    auto beatdummy = std::make_shared<Beat>(200, time, time);
    // 找到第一个大于或等于 beatdummy 的元素
    auto it = beats.lower_bound(beatdummy);
    if (it == beats.end()) {
        // 找不到比当前时间还靠后的拍
        // 看上一拍是否存在(500ms内)
        --it;
        if (std::abs(it->get()->end_timestamp - time) < 500) {
            return {*it};
        } else {
            return {};
        }
    } else {
        if (it->get()->start_timestamp == time) {
            // 鼠标位置已经有一拍
            std::list<std::shared_ptr<Beat>> list = {*it};
            ++it;
            // 检查下一拍
            if (it == beats.end()) {
                // 不存在下一拍(最后一拍了)
            } else {
                // 添加下一拍
                list.emplace_back(*it);
            }

            return list;
        } else {
            // 找到鼠标位置后的下一拍
            if (it == beats.begin()) {
                // 下拍就已经是第一拍
                // 500ms内返回此拍-否则不返回
                if (std::abs(it->get()->start_timestamp - time) < 500) {
                    return {*it};
                } else {
                    return {};
                }
            } else {
                // 返回此拍和上一拍
                std::list<std::shared_ptr<Beat>> list = {*it};
                --it;
                list.emplace_back(*it);
                return list;
            }
        }
    }
}

// 查询时间区间窗口内的拍
void MMap::query_beat_in_range(std::vector<std::shared_ptr<Beat>>& result_beats,
                               int32_t start, const int32_t end) {
    result_beats.clear();
    auto lower_bound = std::make_shared<Beat>(200, start, start);

    // 找到第一个大于或等于 lower_bound 的元素
    auto it = beats.lower_bound(lower_bound);

    // 遍历直到超出 upper_bound
    while (it != beats.end() && (*it)->start_timestamp < end) {
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
    std::multiset<std::shared_ptr<HitObject>, HitObjectComparator>&
        result_objects,
    int32_t start, const int32_t end, bool with_longhold) {
    std::unique_lock<std::mutex> lock(hitobjects_mutex);
    /*
     *lower_bound(key)	返回第一个 ≥ key 的元素的迭代器。
     *upper_bound(key)	返回第一个 > key 的元素的迭代器。
     */
    if (with_longhold) {
        // 带超级长条
        // 面尾时间>窗口起始时间且面头<结束时间就加入列表
        for (const auto& hold : temp_hold_list) {
            if (hold->timestamp < end &&
                hold->timestamp + hold->hold_time > start) {
                result_objects.insert(hold);
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
            result_objects.insert(*startit);
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
        if (note && note->parent_reference &&
            temp_comp != note->parent_reference) {
            temp_comp = note->parent_reference;
            // 将所有子物件加入渲染队列
            for (const auto& subnote : temp_comp->child_notes) {
                result_objects.insert(subnote);
            }
        }
        // 添加窗口内物件
        result_objects.insert(*startit);
    }
}
