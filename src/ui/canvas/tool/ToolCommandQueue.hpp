#ifndef MMM_TOOLCOMMANDQUEUE_HPP
#define MMM_TOOLCOMMANDQUEUE_HPP

#include <queue>
#include <tool/ToolCommand.hpp>

class ToolCommandQueue {
   public:
    // [UI 线程调用] 将一个命令推入队列
    void push(const ToolCommand& command) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(command);
    }

    // [工作线程调用] 一次性取出队列中的所有命令
    // 返回一个vector可以让你在没有锁的情况下处理命令，将锁的持有时间降到最低
    std::vector<ToolCommand> drain() {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_queue.empty()) {
            return {};
        }

        std::vector<ToolCommand> commands;
        commands.reserve(m_queue.size());
        while (!m_queue.empty()) {
            // 使用 move 来提高效率
            commands.push_back(std::move(m_queue.front()));
            m_queue.pop();
        }
        return commands;
    }

   private:
    std::queue<ToolCommand> m_queue;
    mutable std::mutex m_mutex;
};

#endif  // MMM_TOOLCOMMANDQUEUE_HPP
