#include <cstdint>
#include <mmm/obj/Note.hpp>
#include <mmm/obj/rm/Slide.hpp>
#include <nlohmann/json.hpp>

// 打印用
std::string Note::toString() const {
    std::stringstream ss;
    ss << "Note:\n";
    ss << "  Type: " << to_string(type) << "\n";
    ss << "  Time: " << time << "\n";
    ss << "  Track: " << track << "\n";
    return ss.str();
}

// json转换
nlohmann::json Note::toJson() const {
    nlohmann::json data;
    data["type"] = to_string(type);
    data["time"] = time;
    data["track"] = track;
    return data;
}

void Note::fromJson(nlohmann::json& data) {
    time = data["time"].get<uint32_t>();
    track = data["track"].get<uint32_t>();
}

// 从滑键转换
std::list<std::unique_ptr<Note>> Note::from_slide(const Slide* slide) {
    // 在滑动轨迹上生成note
    if (!slide) return {};
    std::list<std::unique_ptr<Note>> res;
    // 从哪个轨道
    auto from = slide->delta_track() < 0 ? slide->track + slide->delta_track()
                                         : slide->track;
    // 到哪个轨道
    auto to = slide->delta_track() < 0 ? slide->track
                                       : slide->track + slide->delta_track();
    for (auto i{from}; i <= to; ++i) {
        // 构造osunote
        auto generated_note = std::make_unique<Note>(slide->map());
        generated_note->set_timestamp(slide->timestamp());
        generated_note->set_trackpos(i);
        // 添加到结果集
        res.push_back(std::move(generated_note));
    }
    return res;
}

std::unique_ptr<Note> Note::clone(const MMap* ref) const {
    auto newnote = std::make_unique<Note>(ref);
    newnote->set_timestamp(time);
    newnote->set_trackpos(track);
    return newnote;
}
