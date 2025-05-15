#ifndef M_THREADSAFEQUEUE_H
#define M_THREADSAFEQUEUE_H

#include <mutex>
#include <queue>

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
};

#endif  // M_THREADSAFEQUEUE_H
