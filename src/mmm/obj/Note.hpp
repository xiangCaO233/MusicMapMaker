#ifndef MMM_NOTE_HPP
#define MMM_NOTE_HPP

#include <cstdint>

// 基本物件

class Note {
   public:
    // 构造Note
    Note();

    // 析构Note
    virtual ~Note();

    inline uint32_t timestamp() const { return time; }

    inline uint32_t trackpos() const { return track; }

    virtual void set_timestamp(uint32_t t) { time = t; }

    virtual void set_trackpos(uint32_t o) { track = o; }

   private:
    // 时间
    uint32_t time;

    // 轨道
    uint32_t track;
};

#endif  // MMM_NOTE_HPP
