#ifndef MMM_OSUHOLD_HPP
#define MMM_OSUHOLD_HPP

#include <mmm/obj/Hold.hpp>
#include <mmm/obj/osu/OsuInfo.hpp>

class OsuHold : public Hold, public OsuNoteMetadata {
   public:
    // 构造OsuHold
    using Hold::Hold;

    // 析构OsuHold
    ~OsuHold() override;

    // 打印用
    std::string toString() override;
};

#endif  // MMM_OSUHOLD_HPP
