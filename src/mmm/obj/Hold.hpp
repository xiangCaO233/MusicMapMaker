#ifndef MMM_HOLD_HPP
#define MMM_HOLD_HPP

#include <mmm/obj/Note.hpp>
#include <nlohmann/json.hpp>

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

    // json转换
    nlohmann::json toJson() const override {
        nlohmann::json data;
        data["type"] = to_string(type);
        data["time"] = time;
        data["track"] = track;
        auto& notedata = data["data"];
        notedata["duration"] = duration_time;
        return data;
    };

    void fromJson(nlohmann::json& data) override {
        type = NoteType::HOLD;
        time = data["time"].get<uint32_t>();
        track = data["track"].get<uint32_t>();
        auto& notedata = data["data"];
        duration_time = notedata["duration"];
    };

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
