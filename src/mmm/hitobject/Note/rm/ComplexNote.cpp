#include "ComplexNote.h"

ComplexNote::ComplexNote(uint32_t time, int32_t orbit_pos)
    : Note(time, orbit_pos) {
    object_type = HitObjectType::RMCOMPLEX;
    note_type = NoteType::COMPLEX;
}

ComplexNote::~ComplexNote() = default;

// 打印用
std::string ComplexNote::toString() {
    return "ComplexNote{timestamp=" + std::to_string(timestamp) +
           ", orbit=" + std::to_string(orbit) +
           ", child_count=" + std::to_string(child_notes.size()) + "}";
}

// 深拷贝
ComplexNote* ComplexNote::clone() {
    auto comp = new ComplexNote(timestamp, orbit);
    for (const auto& child_note : child_notes) {
        auto cloned_child = child_note->clone();
        cloned_child->parent_reference = comp;
        comp->child_notes.insert(std::shared_ptr<Note>(cloned_child));
    }
    return comp;
}

// 判同
bool ComplexNote::equals(const std::shared_ptr<HitObject>& other) const {
    if (!Note::equals(other)) return false;
    auto comp = std::static_pointer_cast<ComplexNote>(other);
    if (child_notes.size() != comp->child_notes.size()) return false;
    auto thisit = child_notes.begin();
    auto otherit = comp->child_notes.begin();
    while (thisit != child_notes.end() && otherit != comp->child_notes.end()) {
        if (!thisit->get()->equals(*otherit)) {
            return false;
        }
        ++thisit;
        ++otherit;
    }
    return true;
}
