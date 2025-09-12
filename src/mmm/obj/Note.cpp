#include <mmm/obj/Note.hpp>

// 打印用
std::string Note::toString() const {
    std::stringstream ss;
    ss << "Note:\n";
    ss << "  Type: " << to_string(type) << "\n";
    ss << "  Time: " << time << "\n";
    ss << "  Track: " << track << "\n";
    return ss.str();
}

std::vector<Note> Note::from_slide(std::shared_ptr<Slide> slide) {
    // TODO(xiang 2025-08-08): 实现滑键转多个物件
    return {};
}
