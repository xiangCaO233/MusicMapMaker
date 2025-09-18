#ifndef MMM_THREADSAFEQUEUE_HPP
#define MMM_THREADSAFEQUEUE_HPP

#include <mutex>
#include <queue>

template <typename T>
class ThreadSafeQueue {
   public:
    // 将一个实例推入队列
    void push(const T& command) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(command);
    }

    // 一次性取出队列中的所有实例
    std::vector<T> drain() {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_queue.empty()) {
            return {};
        }

        std::vector<T> commands;
        commands.reserve(m_queue.size());
        while (!m_queue.empty()) {
            // 使用 move 来提高效率
            commands.push_back(std::move(m_queue.front()));
            m_queue.pop();
        }
        return commands;
    }

   private:
    std::queue<T> m_queue;
    mutable std::mutex m_mutex;
};

#endif  // MMM_THREADSAFEQUEUE_HPP
