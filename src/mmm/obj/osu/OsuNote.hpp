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
    ~OsuNote() override;

    // 打印用
    std::string toString() override;
};

#endif  // MMM_OSUNOTE_HPP
