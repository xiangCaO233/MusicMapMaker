#ifndef MMM_COMPOSITE_HPP
#define MMM_COMPOSITE_HPP

#include <memory>
#include <mmm/obj/Note.hpp>
#include <vector>

class Composite : public Note {
   public:
    // 构造Composite
    using Note::Note;

    // 析构Composite
    ~Composite() override = default;

    // 打印用
    std::string toString() const override;

    // 总时长
    inline uint32_t total_duration() const { return total_duration_time; }

    // 添加子物件
    bool add_child(std::unique_ptr<Note> note);

    // 移除组合键最后的子物件
    std::unique_ptr<Note> pop_back();

    // 访问子物件
    auto& children() const { return child_notes; }

    // 访问子物件
    auto& children() { return child_notes; }

    // 设置时间戳
    void set_timestamp(uint32_t t) override;

    // 设置轨道
    void set_trackpos(uint32_t o) override;

    // 克隆物件
    std::unique_ptr<Note> clone(const MMap* ref) const override;

   private:
    // 子物件
    std::vector<std::unique_ptr<Note>> child_notes;

    // 总持续时间
    uint32_t total_duration_time{0};
};

#endif  // MMM_COMPOSITE_HPP
