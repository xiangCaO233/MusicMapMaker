#ifndef MMM_OSUTIMING_HPP
#define MMM_OSUTIMING_HPP

#include <mmm/info/osu/OsuTimingInfo.hpp>
#include <mmm/timing/Timing.hpp>

class OsuTiming : public Timing, public OsuTimingMetadata {
   public:
    // 构造OsuTiming
    OsuTiming();
    // 析构OsuTiming
    ~OsuTiming() override;

    // 转换为osu的字符串
    std::string to_osu_description() override;

    // 从osu的字符串读取
    void from_osu_description(std::vector<std::string>& description) override;
};

#endif  // MMM_OSUTIMING_HPP
