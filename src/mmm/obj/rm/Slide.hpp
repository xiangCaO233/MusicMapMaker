#ifndef MMM_SLIDE_HPP
#define MMM_SLIDE_HPP

#include <mmm/obj/Note.hpp>
#include <nlohmann/json.hpp>

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
    inline int64_t delta_track() const { return dtrack; }

    // 设置dtrack
    inline void set_delta_track(int64_t delta_track) { dtrack = delta_track; }

    // json转换
    nlohmann::json toJson() const override {
        nlohmann::json data;
        data["type"] = to_string(notetype());
        data["time"] = timestamp();
        data["track"] = trackpos();
        auto& notedata = data["data"];
        notedata["delta-track"] = dtrack;
        return data;
    };

    void fromJson(nlohmann::json& data) override {
        set_notetype(NoteType::SLIDE);
        set_timestamp(data["time"].get<uint32_t>());
        set_trackpos(data["track"].get<uint32_t>());
        auto& notedata = data["data"];
        set_delta_track(notedata["delta-track"]);
    };

    // 克隆物件
    std::unique_ptr<Note> clone(const MMap* ref) const override {
        auto new_note_data = std::make_unique<Slide>(ref);
        new_note_data->set_notetype(NoteType::SLIDE);
        new_note_data->set_timestamp(timestamp());
        new_note_data->set_trackpos(trackpos());
        new_note_data->set_delta_track(delta_track());
        return new_note_data;
    }

   private:
    int64_t dtrack{1};
};

#endif  // MMM_SLIDE_HPP
