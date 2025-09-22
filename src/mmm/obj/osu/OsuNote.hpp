#ifndef MMM_OSUNOTE_HPP
#define MMM_OSUNOTE_HPP

#include <list>
#include <memory>
#include <mmm/info/osu/OsuNoteInfo.hpp>
#include <mmm/obj/Note.hpp>

class Slide;

class OsuNote : public Note, public OsuNoteMetadata {
   public:
    // 构造OsuNote
    using Note::Note;
    OsuNote(const MMap* map, const Note* note);

    // 析构OsuNote
    ~OsuNote() override = default;

    // 从滑键转换
    static std::list<std::unique_ptr<OsuNote>> from_slide(const Slide* slide);

    // 打印用
    std::string toString() const override;

    // 从osu描述加载
    void from_osu_description(const std::vector<std::string>& description,
                              int32_t orbit_count) override;

    // 转化为osu描述
    std::string to_osu_description(int32_t orbit_count) const override;

    // 克隆物件
    std::unique_ptr<Note> clone(const MMap* ref) const override;
};

#endif  // MMM_OSUNOTE_HPP
