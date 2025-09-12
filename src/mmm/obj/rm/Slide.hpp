#ifndef MMM_SLIDE_HPP
#define MMM_SLIDE_HPP

#include <mmm/obj/Note.hpp>

class Slide : public Note {
   public:
    // 构造Slide
    using Note::Note;

    // 析构Slide
    ~Slide() override;

    // 打印用
    std::string toString() const override {
        std::stringstream ss;
        ss << "Slide:\n";
        ss << Note::toString();  // 调用基类方法
        ss << "  Delta Track: " << dtrack << "\n";
        return ss.str();
    }

    // 获取dtrack
    inline uint32_t delta_track() const { return dtrack; }

    // 设置dtrack
    inline void set_track_orbit(uint32_t delta_track) { dtrack = delta_track; }

   private:
    uint32_t dtrack{1};
};

#endif  // MMM_SLIDE_HPP
