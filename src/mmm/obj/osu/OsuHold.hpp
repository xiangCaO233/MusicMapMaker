#ifndef MMM_OSUHOLD_HPP
#define MMM_OSUHOLD_HPP

#include <mmm/info/osu/OsuNoteInfo.hpp>
#include <mmm/obj/Hold.hpp>

class OsuHold : public Hold, public OsuNoteMetadata {
   public:
    // 构造OsuHold
    using Hold::Hold;

    // 析构OsuHold
    ~OsuHold() override = default;

    // 打印用
    std::string toString() const override;

    // 克隆物件
    std::unique_ptr<Note> clone(const MMap* ref) const override;

    // 从osu描述加载
    void from_osu_description(const std::vector<std::string>& description,
                              int32_t orbit_count) override;

    // 转化为osu描述
    std::string to_osu_description(int32_t orbit_count) override;
};

#endif  // MMM_OSUHOLD_HPP
