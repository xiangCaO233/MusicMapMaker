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

    inline uint32_t orbitpos() const { return time; }

    virtual void set_timestamp(uint32_t t) { time = t; }

    virtual void set_orbitpos(uint32_t o) { orbit = o; }

   private:
    // 时间
    uint32_t time;

    // 轨道
    uint32_t orbit;
};

#endif  // MMM_NOTE_HPP
