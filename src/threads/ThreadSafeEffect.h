#ifndef M_THREADSAFEQUEUE_H
#define M_THREADSAFEQUEUE_H

#include <cstdint>
#include <mutex>
#include <queue>

enum class EffectType {
    NORMAL,
    SLIDEARROW,
};

// 可锁特效-线程安全
class ThreadSafeEffect {
   public:
    // 构造ThreadSafeEffect
    ThreadSafeEffect() = default;
    // 析构ThreadSafeEffect
    virtual ~ThreadSafeEffect() = default;
    std::mutex mtx;

    // 特效类型
    EffectType effect_type;
    // 剩余时间
    double time_left;
    // 当前播放到的帧位置
    int32_t current_frame_pos;
    // 播放位置
    double xpos;
    double ypos;
};

#endif  // M_THREADSAFEQUEUE_H
