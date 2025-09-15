#include <mmm/obj/Hold.hpp>
#include <mmm/obj/Note.hpp>
#include <mmm/obj/rm/Composite.hpp>
#include <mmm/obj/rm/Slide.hpp>

// 打印用
std::string Composite::toString() const {
    std::stringstream ss;
    ss << "Composite:\n";
    ss << Note::toString();  // 调用基类方法
    ss << "  Total Duration: " << total_duration_time << "\n";
    ss << "  Children: " << child_notes.size() << "\n";

    // 遍历所有子音符，并递归调用它们的 toString() 方法
    for (size_t i = 0; i < child_notes.size(); ++i) {
        ss << "    --- Child #" << i << " ---\n";
        // 缩进子音符的打印结果，增强可读性
        std::string child_str = child_notes[i]->toString();

        // 替换子字符串中的换行符以添加缩进
        size_t pos = 0;
        std::string tabbed_str = "    ";  // 额外的缩进
        while ((pos = child_str.find('\n', pos)) != std::string::npos) {
            child_str.replace(pos, 1, "\n" + tabbed_str);
            pos += tabbed_str.length() + 1;
        }

        ss << tabbed_str << child_str;

        ss << "\n";
    }

    return ss.str();
}

// 添加子物件
bool Composite::add_child(std::unique_ptr<Note> note) {
    bool success{false};
    if (child_notes.empty()) {
        if (note->notetype() == NoteType::HOLD ||
            note->notetype() == NoteType::SLIDE) {
            // 只可存在滑键和长条
            success = true;
        }
    } else {
        auto end = child_notes.back().get();
        switch (end->notetype()) {
            case NoteType::HOLD: {
                // 当前组合键结尾是面条
                if (auto hold = static_cast<Hold*>(end);
                    note->notetype() == NoteType::SLIDE &&
                    hold->timestamp() + hold->duration() == note->timestamp()) {
                    // 新添加的必须是滑键且时间戳在末尾面条的结尾处
                    success = true;
                }
                break;
            }
            case NoteType::SLIDE: {
                // 当前组合键结尾是滑键
                if (auto slide = static_cast<Slide*>(end);
                    note->notetype() == NoteType::HOLD &&
                    slide->trackpos() + slide->delta_track() ==
                        note->trackpos() &&
                    slide->timestamp() == note->timestamp()) {
                    // 新添加的必须是面条且轨道在滑键的结尾轨道处且时间必须相同
                    success = true;
                }
                break;
            }
            default: {
                // 结尾物件是其他类型物件则出错
                break;
            }
        }
    }
    if (success) {
        child_notes.push_back(std::move(note));
    }
    return success;
}

// 取出组合键最后的子物件
std::unique_ptr<Note> Composite::pop_back() {
    if (child_notes.empty()) {
        return nullptr;
    }
    auto last_note = std::move(child_notes.back());
    child_notes.pop_back();
    return last_note;
}

// 设置时间戳
void Composite::set_timestamp(uint32_t t) {
    // 递归更新所有子物件的时间戳
    auto oldtime = timestamp();
    for (auto& child_note : child_notes) {
        auto delta_time = child_note->timestamp() - oldtime;
        child_note->set_timestamp(t + delta_time);
    }
    time = t;
}

// 设置轨道
void Composite::set_trackpos(uint32_t o) {
    // 递归更新所有子物件的轨道
    auto oldtrack = trackpos();
    for (auto& child_note : child_notes) {
        auto delta_track = child_note->trackpos() - oldtrack;
        child_note->set_trackpos(o + delta_track);
    }
    track = o;
}

// 克隆物件
std::unique_ptr<Note> Composite::clone(MMap* ref) const {
    auto new_note_data = std::make_unique<Composite>(ref);
    auto new_composite = static_cast<Composite*>(new_note_data.get());
    for (const auto& old_child : children()) {
        std::unique_ptr<Note> new_child{nullptr};
        switch (old_child->notetype()) {
            case NoteType::HOLD: {
                new_child = std::make_unique<Hold>(ref);
                new_child->set_notetype(NoteType::HOLD);
                break;
            }
            case NoteType::SLIDE: {
                new_child = std::make_unique<Slide>(ref);
                new_child->set_notetype(NoteType::SLIDE);
                break;
            }
            case NoteType::NORMAL:
            case NoteType::COMPOSITE:
                break;
        }
        new_composite->add_child(std::move(new_child));
    }
    return new_note_data;
}
