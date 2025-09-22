#ifndef MMM_OSUTIMING_HPP
#define MMM_OSUTIMING_HPP

#include <mmm/info/osu/OsuTimingInfo.hpp>
#include <mmm/timing/Timing.hpp>

class OsuTiming : public Timing, public OsuTimingMetadata {
   public:
    // 构造OsuTiming
    OsuTiming() = default;
    OsuTiming(Timing* source);
    // 析构OsuTiming
    ~OsuTiming() override = default;

    // 转换为osu的字符串
    std::string to_osu_description() override;

    // 从osu的字符串读取
    void from_osu_description(std::vector<std::string>& description) override;

    std::unique_ptr<Timing> clone() const override;
};

#endif  // MMM_OSUTIMING_HPP
