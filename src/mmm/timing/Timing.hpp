#ifndef MMM_TIMING_HPP
#define MMM_TIMING_HPP

#include <cstdint>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>

enum class TimingType {
    GENERAL,
    OSUTIMING,
    RMTIMING,
    MALODYTIMING,
};

class Timing {
   public:
    // 构造Timing
    Timing() = default;
    // 析构Timing
    virtual ~Timing() = default;

    TimingType type{TimingType::GENERAL};

    // 是否为基准timing
    bool is_base_timing{true};

    // 该时间点的时间戳
    // in-osu
    // *时间（整型）：
    // *时间轴区间的开始时间，以谱面音频开始为原点，单位是毫秒。
    // 这个时间轴区间的结束时间即为下一个时间点的开始时间（如果这是最后一个时间点，则无结束时间）。
    // in-imd
    //
    uint32_t timestamp;

    // 该时间点的bpm(始终都携带最近的红线的歌曲bpm)
    //
    double bpm{-1.0};

    ///< 拍长(ms)或滑条速度倍率(负值)
    double beat_length{0};

    // json转换
    virtual nlohmann::json toJson() const {
        nlohmann::json timing_json;
        timing_json["time"] = timestamp;
        timing_json["bpm"] = bpm;
        timing_json["isbase"] = is_base_timing;
        timing_json["beatlength"] = beat_length;
        // TODO(xiang 2025-05-19): timing的元数据实现和保存
        return timing_json;
    };

    virtual void fromJson(nlohmann::json& data) {
        timestamp = data["time"].get<uint32_t>();
        bpm = data["bpm"].get<double>();
        beat_length = data["beatlength"].get<double>();
        is_base_timing = data["isbase"].get<bool>();
        // TODO(xiang 2025-05-19): timing的元数据实现和保存
    };

    virtual std::unique_ptr<Timing> clone() const {
        auto newTiming = std::make_unique<Timing>();
        newTiming->timestamp = timestamp;
        newTiming->bpm = bpm;
        newTiming->beat_length = beat_length;
        newTiming->type = type;
        return newTiming;
    }

    // 增加一个辅助的 to_string
    virtual std::string to_string() const {
        return "Timing(ts=" + std::to_string(timestamp) + ", " +
               (is_base_timing ? "Base" : "Inherited") +
               ", val=" + std::to_string(beat_length) + ")";
    }

    bool operator==(const Timing& other) const {
        return timestamp == other.timestamp &&
               is_base_timing == other.is_base_timing && type == other.type &&
               bpm == other.bpm && beat_length == other.beat_length;
    }
};

#endif  // MMM_TIMING_HPP
