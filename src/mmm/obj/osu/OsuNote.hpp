#ifndef MMM_OSUNOTE_HPP
#define MMM_OSUNOTE_HPP

#include <mmm/info/osu/OsuNoteInfo.hpp>
#include <mmm/obj/Note.hpp>

class Slide;

class OsuNote : public Note, public OsuNoteMetadata {
   public:
    // 构造OsuNote
    using Note::Note;

    // 析构OsuNote
    ~OsuNote() override = default;

    // 打印用
    std::string toString() const override;

    // 从osu描述加载
    void from_osu_description(const std::vector<std::string>& description,
                              int32_t orbit_count) override;

    // 转化为osu描述
    std::string to_osu_description(int32_t orbit_count) override;
};

#endif  // MMM_OSUNOTE_HPP
