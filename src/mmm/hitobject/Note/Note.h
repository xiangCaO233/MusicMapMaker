#ifndef M_NOTE_H
#define M_NOTE_H

#include <memory>

#include "../../Metadata.h"
#include "../HitObject.h"

class ComplexNote;

enum class NoteType : uint8_t {
    // 另外的滑键和折线
    COMPLEX = 0x03,
    SLIDE = 0x01,
    // osu!mania的单键和长条
    HOLD = 0x02,
    NOTE = 0x00,
};

enum class ComplexInfo : uint8_t {
    NONE = 0,
    HEAD = 0x60,
    BODY = 0x20,
    END = 0xa0,
};

class Note : public HitObject {
   public:
    // 构造Note
    explicit Note(int32_t time, int32_t orbit_pos);
    // 析构Note
    ~Note() override;

    // 物件类型
    NoteType note_type;

    // 元数据集
    std::unordered_map<NoteMetadataType, std::shared_ptr<NoteMetadata>>
        metadatas;

    // 物件所处轨道
    int32_t orbit;

    // 父组合键引用
    ComplexNote* parent_reference{nullptr};

    // 组合键分区
    ComplexInfo compinfo{ComplexInfo::NONE};

    // 接收处理
    void accept_generate(ObjectGenerator& generator) override;
    void accept_generate_preview(ObjectGenerator& generator) override;

    // 打印用
    std::string toString() override;

    // 深拷贝
    virtual Note* clone() override;

    // 是否为相同物件
    virtual bool equals(const std::shared_ptr<HitObject>& other) const override;

    // 比较器使用
    bool lessThan(const HitObject* other) const override;
};

#endif  // M_NOTE_H
