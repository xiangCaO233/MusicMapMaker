#ifndef MMM_SLIDE_HPP
#define MMM_SLIDE_HPP

#include <mmm/obj/Note.hpp>

class Slide : public Note {
   public:
    // 构造Slide
    using Note::Note;

    // 析构Slide
    ~Slide() override = default;

    // 打印用
    std::string toString() const override {
        std::stringstream ss;
        ss << "Slide:\n";
        ss << Note::toString();  // 调用基类方法
        ss << "  Delta Track: " << dtrack << "\n";
        return ss.str();
    }

    // 获取dtrack
    inline uint32_t delta_track() const { return dtrack; }

    // 设置dtrack
    inline void set_track_orbit(uint32_t delta_track) { dtrack = delta_track; }

    // 克隆物件
    std::unique_ptr<Note> clone(MMap* ref) const override {
        auto new_note_data = std::make_unique<Slide>(ref);
        new_note_data->set_notetype(NoteType::SLIDE);
        new_note_data->set_timestamp(timestamp());
        new_note_data->set_trackpos(trackpos());
        new_note_data->set_track_orbit(delta_track());
        return new_note_data;
    }

   private:
    uint32_t dtrack{1};
};

#endif  // MMM_SLIDE_HPP
