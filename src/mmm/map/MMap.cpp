#include <colorful-log.h>

#include <QDebug>
#include <action/modules/canvas/EditorActionHandler.hpp>
#include <algorithm>
#include <memory>
#include <mmm/map/MMap.hpp>
#include <mmm/map/editor/MMapEditor.hpp>
#include <util/mutil.hpp>

MMap::MMap() {}

MMap::MMap(std::string_view file) {
    basemeta.map_path = std::string(file);
    if (file.ends_with(".osu")) {
        // 读取osu
        readOsu();
    } else if (file.ends_with(".imd")) {
        readImd();
    } else if (file.ends_with(".mmm")) {
        readMMM();
    }
}

MMap::~MMap() = default;

// 写出到文件
void MMap::writeOut(const std::string& file) {
    if (file.ends_with(".osu")) {
        // 写出为osu
        writeOsu(file);
    } else if (file.ends_with(".imd")) {
        writeImd(file);
    } else if (file.ends_with(".mmm")) {
        writeMMM(file);
    }
}

// 注册编辑器
void MMap::register_editor(ThreadSafeQueue<MMapEditEvent>& editEventQueue) {
    std::lock_guard<std::mutex> lock(editor_mutex);
    if (!mapeditor) {
        // 初始化对应编辑器
        mapeditor = std::make_shared<MMapEditor>(this, editEventQueue);
    }

    auto mapeditor_ref = mapeditor.get();
    // todo:断开之前的编辑器信号
    // 连接编辑器信号
    connect(EditorActionHandler::instance(), &EditorActionHandler::undo,
            [mapeditor_ref]() { mapeditor_ref->undo(); });
    connect(EditorActionHandler::instance(), &EditorActionHandler::redo,
            [mapeditor_ref]() { mapeditor_ref->redo(); });
}

// 访问谱面元数据
std::weak_ptr<MapMetadata> MMap::map_metadata(MapMetadataType type) {
    auto metaptr_it = metadatas.find(type);
    if (metaptr_it == metadatas.end()) {
        return {};
    }
    return metaptr_it->second;
}

// 根据timing数据更新全图参考bpm
void MMap::update_preferenceBPM() {
    bool finded{false};
    // 读取全图参考bpm
    for (const auto& [time, timings] : timing_set().get_all_timing_points()) {
        for (const auto& timing : timings) {
            // 使用第一个不带变速的绝对bpm
            if (timing->is_base_timing) {
                basemeta.preference_bpm = timing->bpm;
                finded = true;
                break;
            }
        }
        if (finded) {
            break;
        }
    }

    // 再没找到就用第一个timing的绝对bpm-没有用200
    if (!finded) {
        if (timing_set().get_all_timing_points().empty()) {
            basemeta.preference_bpm = 200;
        } else {
            basemeta.preference_bpm = timing_set()
                                          .get_all_timing_points()
                                          .begin()
                                          ->second.begin()
                                          ->get()
                                          ->bpm;
        }
    }
}

// 更新拍信息(智能识别分拍)
void MMap::analyzeBeatInfo() {
    // 从 TimingMap 筛选出所有基础Timing点(红线)
    std::vector<const Timing*> base_timings;
    const auto& all_timings_map = timings.get_all_timing_points();
    for (const auto& pair : all_timings_map) {
        // pair.first is timestamp, pair.second is std::vector<Timing>
        for (const auto& t : pair.second) {
            if (t->is_base_timing && t->beat_length > 0) {
                base_timings.push_back(t.get());
                // 每个时间点只有一个红线
                break;
            }
        }
    }
    if (base_timings.empty()) {
        // 如果需要，可以在此输出警告
        XWARN("无红线/分析结束");
        return;
    }

    // 确定谱面分析的结束时间
    int64_t map_end_time = basemeta.map_length;
    if (map_end_time == 0 && !base_timings.empty()) {
        map_end_time = base_timings.back()->timestamp +
                       static_cast<int64_t>(base_timings.back()->beat_length);
    }

    // 遍历每个Timing区段
    for (size_t i = 0; i < base_timings.size(); ++i) {
        const auto& current_timing = base_timings[i];
        double original_beat_length = current_timing->beat_length;

        // --- 安全保护：检查 beat_length 是否在合理范围内 ---
        if (original_beat_length < 50) {
            XWARN("timing的bpm过大/鉴定为特效timing,跳过(防止炸内存)");
            continue;  // 跳过此 timing 点控制的整个区段
        }
        if (original_beat_length > 5000.0) {
            XWARN("timing的bpm过小[" + std::to_string(current_timing->bpm) +
                  "]/鉴定为特效timing,限定到5000ms,等效bpm:[12]分析");
        }

        auto generation_beat_length = std::min(original_beat_length, 5000.0);

        // 确定此区段的结束时间
        auto section_end_time = (i + 1 < base_timings.size())
                                    ? base_timings[i + 1]->timestamp
                                    : map_end_time;

        // 在当前区段内生成并分析所有的拍
        for (double beat_start_double = current_timing->timestamp;
             beat_start_double < section_end_time;
             beat_start_double += generation_beat_length) {
            Beat current_beat;
            current_beat.beat_start =
                static_cast<int64_t>(std::round(beat_start_double));
            current_beat.beat_length = generation_beat_length;

            if (current_beat.beat_length < 1e-6) continue;  // 避免0长度

            // 调用分析函数 (假设 calculateDivisionStrategy
            // 也已更新以使用新的 Beat 结构) 这一步无需修改，因为它内部的
            // notes.query_range 调用是正确的
            int division =
                mutil::calculateDivisionStrategy(notes, current_beat, 5);
            if (division == -1) {
                // 分析失败,不算
                division = 2;
                current_beat.is_manual = false;
            } else {
                // 分析出来的-算固定分拍
                current_beat.is_manual = true;
            }

            // 填充 BeatInfo
            current_beat.divisors = division;
            current_beat.timing = current_timing;
            current_beat.beat_index = beatTimeline.size();
            beatInfo[current_beat.beat_start] = current_beat;

            beatTimeline.emplace_back(current_beat.beat_start);
        }
    }
}
