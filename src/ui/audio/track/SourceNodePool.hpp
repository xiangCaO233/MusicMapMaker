#ifndef MMM_SOURCENODEPOOL_HPP
#define MMM_SOURCENODEPOOL_HPP

#include <ice/core/PlayCallBack.hpp>
#include <ice/core/SourceNode.hpp>
#include <mutex>
#include <queue>

// 封装SourceNode池
class SourceNodePool {
   public:
    class OneShotPlayCallback : public ice::PlayCallBack {
       public:
        // 构造函数，需要知道所属的池子和对应的 SourceNode
        OneShotPlayCallback(std::shared_ptr<SourceNodePool> pool,
                            std::shared_ptr<ice::SourceNode> node)
            : m_pool(pool), m_node(node) {}
        // 播放完成完整一遍回调(传入是否循环)
        void play_done(bool loop) const override {
            // 如果不是循环播放，将 SourceNode 归还给池子
            if (!loop) {
                if (auto pool = m_pool.lock()) {
                    if (auto node = m_node.lock()) {
                        // 重置播放位置
                        node->set_playpos(0);
                        pool->release_node(node);
                    }
                }
            }
        };

        // 帧基
        virtual void frameplaypos_updated(size_t frame_pos) override {};

        // 时间基
        virtual void timeplaypos_updated(
            std::chrono::nanoseconds time_pos) override {};

       private:
        // 使用 weak_ptr 避免 SourceNodePool 和 SourceNodePoolCallback
        // 之间的循环引用
        std::weak_ptr<SourceNodePool> m_pool;
        std::weak_ptr<ice::SourceNode> m_node;
    };

   private:
    std::queue<std::shared_ptr<ice::SourceNode>> ready_queue;
    std::mutex mtx;

   public:
    // 获取一个可用的 SourceNode，如果没有则返回 nullptr
    std::shared_ptr<ice::SourceNode> get_node() {
        std::unique_lock<std::mutex> lock(mtx);
        if (ready_queue.empty()) {
            return nullptr;
        }
        auto node = ready_queue.front();
        ready_queue.pop();
        return node;
    }

    // 将 SourceNode 归还到池中
    void release_node(std::shared_ptr<ice::SourceNode> node) {
        std::unique_lock<std::mutex> lock(mtx);
        ready_queue.push(node);
    }
};
#endif  // MMM_SOURCENODEPOOL_HPP
