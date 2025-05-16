#ifndef M_OSUHOLD_H
#define M_OSUHOLD_H

#include <memory>
#include <vector>

#include "../Hold.h"
#include "../HoldEnd.h"
#include "OsuInfo.h"

class OsuHold : public Hold {
   public:
    // 构造OsuHold
    OsuHold(uint32_t time, int32_t orbit_pos, uint32_t holdtime);
    // 从父类构造-填充属性
    explicit OsuHold(std::shared_ptr<Hold> src);
    OsuHold();
    // 析构OsuHold
    ~OsuHold() override;

    // note采样
    NoteSample sample;

    // 物件音效组
    NoteSampleGroup sample_group;

    // 打印用
    std::string toString() override;

    // 从osu描述加载
    void from_osu_description(std::vector<std::string>& description,
                              int32_t orbit_count);

    // 转化为osu描述
    std::string to_osu_description(int32_t orbit_count);
};

// 面尾
class OsuHoldEnd : public HoldEnd {
   public:
    // 构造OsuHoldEnd
    explicit OsuHoldEnd(const std::shared_ptr<OsuHold>& ohold);
    // 析构OsuHoldEnd
    ~OsuHoldEnd() override;

    // 对应面条物件引用
    OsuHold* reference;

    // 打印用
    std::string toString() override;
};

#endif  // M_OSUHOLD_H
