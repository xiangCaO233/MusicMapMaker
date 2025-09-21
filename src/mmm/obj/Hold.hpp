#ifndef MMM_HOLD_HPP
#define MMM_HOLD_HPP

#include <mmm/obj/Note.hpp>

class Hold : public Note {
   public:
    // 构造Hold
    using Note::Note;
    // 析构Hold
    ~Hold() override = default;

    // 打印用
    std::string toString() const override {
        std::stringstream ss;
        ss << "Hold:\n";
        ss << Note::toString();
        ss << "  Duration: " << duration_time << "\n";
        return ss.str();
    };

    inline uint32_t duration() const { return duration_time; }

    inline void set_duration(uint32_t duration) { duration_time = duration; }

    // 克隆物件
    std::unique_ptr<Note> clone(const MMap* ref) const override {
        auto new_note_data = std::make_unique<Hold>(ref);
        new_note_data->set_notetype(NoteType::HOLD);
        new_note_data->set_timestamp(timestamp());
        new_note_data->set_trackpos(trackpos());
        new_note_data->set_duration(duration_time);
        return new_note_data;
    };

   private:
    uint32_t duration_time{0};
    friend class OsuHold;
};

#endif  // MMM_HOLD_HPP
