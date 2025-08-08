#include <mmm/obj/Hold.hpp>
#include <mmm/obj/rm/Composite.hpp>
#include <mmm/obj/rm/Slide.hpp>

// 析构Composite
Composite::~Composite() = default;

// 打印用
std::string Composite::toString() {}

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
