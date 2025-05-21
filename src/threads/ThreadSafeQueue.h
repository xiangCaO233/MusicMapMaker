#ifndef M_THREADSAFEQUEUE_H
#define M_THREADSAFEQUEUE_H

#include <cstdint>
#include <mutex>
#include <queue>

enum class EffectType {
    NORMAL,
    SLIDEARROW,
};

// 可锁队列-线程安全
template <typename T>
class ThreadSafeQueue {
   public:
    // 构造ThreadSafeQueue
    ThreadSafeQueue() = default;
    // 析构ThreadSafeQueue
    virtual ~ThreadSafeQueue() = default;

    std::queue<T> queue;
    std::mutex mtx;

    // 特效类型
    EffectType effect_type;
    // 剩余时间
    double time_left;
    // 当前播放到的帧位置
    int32_t current_frame_pos;
};

#endif  // M_THREADSAFEQUEUE_H
