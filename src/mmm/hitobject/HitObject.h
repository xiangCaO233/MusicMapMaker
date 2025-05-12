#ifndef M_HITOBJECT_H
#define M_HITOBJECT_H

#include <cstdint>
#include <memory>
#include <string>

class ObjectGenerator;
class Beat;

// 基本打击物件---之后可以实现更多音游的物件
enum class HitObjectType {
    NOTE,
    HOLD,
    HOLDEND,
    OSUNOTE,
    OSUHOLD,
    OSUHOLDEND,
    RMSLIDE,
    RMCOMPLEX,
};

// 打击物件
class HitObject {
   public:
    // 构造HitObject
    explicit HitObject(uint32_t time);
    // 析构HitObject
    virtual ~HitObject();

    // 物件具体类型-可直接static转化
    HitObjectType object_type;

    // 是否为note
    bool is_note{false};

    // 是否是面尾物件
    bool is_hold_end{false};

    // 物件时间戳
    int32_t timestamp;

    // 拍信息
    std::shared_ptr<Beat> beatinfo;

    // 位置
    int32_t divpos{0};

    // 接收处理
    virtual void accept_generate(ObjectGenerator& generator) = 0;
    virtual void accept_generate_preview(ObjectGenerator& generator) = 0;

    // 打印用
    virtual std::string toString() = 0;

    // 深拷贝
    virtual HitObject* clone() = 0;

    // 判同
    virtual bool equals(const std::shared_ptr<HitObject>& other) const = 0;

    // 比较器使用
    virtual bool lessThan(const HitObject* other) const = 0;
};

// 物件比较器
struct HitObjectComparator {
    bool operator()(const std::shared_ptr<HitObject>& a,
                    const std::shared_ptr<HitObject>& b) const {
        if (a->timestamp != b->timestamp) {
            return a->timestamp < b->timestamp;
        } else {
            return a->object_type > b->object_type;
        }
    }
};

#endif  // M_HITOBJECT_H
