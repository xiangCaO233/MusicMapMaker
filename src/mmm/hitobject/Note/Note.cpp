#include "Note.h"

#include <typeinfo>

#include "../../canvas/map/generator/ObjectGenerator.h"

Note::Note(uint32_t time, int32_t orbit_pos)
    : HitObject(time), orbit(orbit_pos) {
    is_note = true;

    object_type = HitObjectType::NOTE;
    note_type = NoteType::NOTE;
}

Note::~Note() {}

// 深拷贝
Note* Note::clone() {
    auto note = new Note(timestamp, orbit);
    note->beatinfo = beatinfo;
    note->compinfo = compinfo;
    return note;
}

// 是否为相同物件
bool Note::equals(const std::shared_ptr<HitObject>& other) const {
    auto onote = std::dynamic_pointer_cast<Note>(other);
    if (!onote) return false;
    if (note_type != onote->note_type) return false;
    return std::abs(timestamp - onote->timestamp) < 5 && orbit == onote->orbit;
}

// 接收处理
void Note::accept_generate(ObjectGenerator& generator) {
    generator.generate(*this);
}
void Note::accept_generate_preview(ObjectGenerator& generator) {
    generator.generate_preview(*this);
}

// 打印用
std::string Note::toString() {
    std::string typeStr;
    switch (note_type) {
        case NoteType::COMPLEX:
            typeStr = "COMPLEX";
            break;
        case NoteType::SLIDE:
            typeStr = "SLIDE";
            break;
        case NoteType::HOLD:
            typeStr = "HOLD";
            break;
        case NoteType::NOTE:
            typeStr = "NOTE";
            break;
    }

    std::string compStr;
    switch (compinfo) {
        case ComplexInfo::NONE:
            compStr = "NONE";
            break;
        case ComplexInfo::HEAD:
            compStr = "HEAD";
            break;
        case ComplexInfo::BODY:
            compStr = "BODY";
            break;
        case ComplexInfo::END:
            compStr = "END";
            break;
    }

    return "Note{timestamp=" + std::to_string(timestamp) +
           ", orbit=" + std::to_string(orbit) + ", type=" + typeStr +
           ", compinfo=" + compStr + "}";
}

// 比较器使用
bool Note::lessThan(const HitObject* other) const {
    if (typeid(*this) != typeid(*other)) {
        // 类型不同
        return timestamp < other->timestamp
                   ? true
                   : this->object_type < other->object_type;
    } else {
        // 比较内容
        auto otherd = static_cast<const Note*>(other);
        if (timestamp < other->timestamp) {
            return true;
        } else {
            if (timestamp == other->timestamp) {
                // 二级比较轨道
                return orbit < otherd->orbit;
            } else {
                return false;
            }
        }
    }
}
