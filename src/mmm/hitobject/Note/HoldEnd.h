#ifndef M_HOLDEND_H
#define M_HOLDEND_H

#include <memory>

#include "Hold.h"

// 面尾
class HoldEnd : public HitObject {
   public:
    // 构造HoldEnd
    HoldEnd(const std::shared_ptr<Hold>& hold);
    HoldEnd();
    // 析构HoldEnd
    ~HoldEnd() override;

    // 对应面条物件引用
    Hold* reference;

    // 接收处理
    void accept_generate(ObjectGenerator& generator) override;
    void accept_generate_preview(ObjectGenerator& generator) override;

    // 打印用
    std::string toString() override;

    // 深拷贝
    HoldEnd* clone() override;

    // 是否为相同物件
    bool equals(const std::shared_ptr<HitObject>& other) const override;

    // 比较器使用
    bool lessThan(const HitObject* other) const override;
};

#endif  // M_HOLDEND_H
