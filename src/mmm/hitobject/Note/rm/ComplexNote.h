#ifndef M_COMPLEXNOTE_H
#define M_COMPLEXNOTE_H

#include <memory>
#include <set>

#include "../Note.h"
#include "mmm/hitobject/HitObject.h"

// 组合键
class ComplexNote : public Note {
   public:
    // 构造ComplexNote
    explicit ComplexNote(uint32_t time, int32_t orbit_pos);
    // 析构ComplexNote
    ~ComplexNote() override;

    // 所有子物件
    std::multiset<std::shared_ptr<Note>, HitObjectComparator> child_notes;

    // 打印用
    std::string toString() override;

    // 深拷贝
    virtual ComplexNote* clone() override;

    // 判同
    virtual bool equals(const std::shared_ptr<HitObject>& other) const override;
};

#endif  // M_COMPLEXNOTE_H
