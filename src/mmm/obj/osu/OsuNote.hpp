#ifndef MMM_OSUNOTE_HPP
#define MMM_OSUNOTE_HPP

#include <mmm/obj/Note.hpp>
#include <mmm/obj/osu/OsuInfo.hpp>

class Slide;

class OsuNote : public Note, public OsuNoteMetadata {
   public:
    // 构造OsuNote
    using Note::Note;

    // 析构OsuNote
    ~OsuNote() override;

    // 打印用
    std::string toString() override;
};

#endif  // MMM_OSUNOTE_HPP
