#ifndef M_THREADPOOL_H
#define M_THREADPOOL_H

#include <condition_variable>
#include <cstdint>
#include <functional>
#include <future>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool {
   public:
    // 构造ThreadPool
    ThreadPool(int32_t num_threads) {
        // 至少需要一个线程
        if (num_threads == 0) {
            // 获取硬件支持的并发线程数作为默认值
            num_threads = std::thread::hardware_concurrency();
        }

        // 极端情况下的回退至少4线程
        if (num_threads < 4) num_threads = 4;

        // 循环创建工作线程
        for (size_t i = 0; i < num_threads; ++i) {
            // emplace_back 直接在 vector 尾部构造 std::thread 对象
            // 每个线程执行一个 lambda 函数，即工作线程的循环体
            workers.emplace_back([this] {
                // 工作线程的无限循环，直到线程池被标记为停止且任务队列为空
                while (true) {
                    // 用于存储从队列中取出的任务
                    std::function<void()> task;
                    {
                        // 临界区开始：访问任务队列
                        // 锁住互斥量
                        std::unique_lock<std::mutex> lock(this->queue_mutex);

                        // 等待条件：
                        // 1. 线程池被标记为停止 (stop_ 为 true)
                        // 2. 或者任务队列不为空 (tasks_ 非空)
                        // condition_variable::wait
                        // 会原子地释放锁并阻塞当前线程， 直到被其他线程 notify
                        // 并且 lambda 返回 true。 如果 lambda 返回 true，则
                        // wait 立即返回，不阻塞。
                        this->condition.wait(lock, [this] {
                            return this->stop || !this->tasks.empty();
                        });

                        // 如果线程池已停止并且任务队列也为空，则工作线程可以安全退出
                        if (this->stop && this->tasks.empty()) {
                            // 退出 lambda，从而结束线程
                            return;
                        }

                        // 如果队列不为空，则取出队首任务
                        // （即使 stop_ 为
                        // true，如果队列中还有任务，也应执行完）
                        if (!this->tasks.empty()) {
                            // 使用 std::move 提高效率
                            task = std::move(this->tasks.front());
                            // 从队列中移除任务
                            this->tasks.pop();
                        } else {
                            // 理论上，如果 stop_ 为 false，wait 不应该在 tasks_
                            // 为空时返回 但为了健壮性，可以继续循环等待
                            continue;
                        }
                        // 临界区结束：unique_lock 析构时自动解锁
                    }

                    // 执行取出的任务 (在锁之外执行，避免阻塞其他线程访问队列)
                    if (task) {
                        // 确保 task 有效
                        task();
                    }
                }
            });
        }
    }

    // 析构ThreadPool
    // 析构函数：停止所有工作线程并等待它们完成
    virtual ~ThreadPool() {
        {
            // 临界区开始：修改 stop_ 标志
            std::unique_lock<std::mutex> lock(queue_mutex);
            // 标记线程池为停止状态
            stop = true;
            // 临界区结束
        }

        // 通知所有等待的工作线程
        // 这样，如果它们因为队列为空而阻塞在 condition_.wait()，会被唤醒
        // 然后它们会检查到 stop_ 为 true 且 tasks_ 为空，从而退出
        condition.notify_all();

        // 等待所有工作线程执行完毕并退出
        // std::thread::join() 会阻塞当前线程（即调用析构函数的线程），
        // 直到目标线程执行完成。
        for (std::thread& worker : workers) {
            if (worker.joinable()) {
                // 确保线程是可 join 的
                worker.join();
            }
        }
    }

    // 提交任务到任务队列 (通用版本，接受任何可调用对象)
    // F: 可调用对象的类型 (如 lambda, 函数指针, std::function)
    // Args: 可调用对象的参数类型
    // 返回一个 std::future<ResultType>，允许异步获取任务结果
    // (本简化版不直接使用其返回值)
    template <class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::invoke_result<F, Args...>::type>
    // C++17 invoke_result
    // C++11/14: std::result_of<F(Args...)>::type
    {
        // 获取任务的返回类型
        using return_type = typename std::invoke_result<F, Args...>::type;

        // 创建一个 std::packaged_task，它包装了原始任务 f 和其参数 args
        // std::packaged_task 可以与 std::future 关联，用于获取异步任务的结果
        auto task_ptr = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        // std::bind 用于将函数 f 和其参数 args 绑定为一个无参数的可调用对象
        // std::forward 用于完美转发参数，保持其原始的左值/右值属性

        // 获取与 packaged_task 关联的 future 对象
        std::future<return_type> res = task_ptr->get_future();

        {
            // _临界区开始：向任务队列添加任务
            std::unique_lock<std::mutex> lock(queue_mutex);

            // 如果线程池已停止，则不允许再添加新任务
            if (stop) {
                throw std::runtime_error("enqueue on stopped TaskExecutor");
            }

            // 将 packaged_task (通过 lambda 包装) 添加到任务队列
            // 工作线程将执行这个 lambda，从而执行 packaged_task
            tasks.emplace([task_ptr]() { (*task_ptr)(); });
            // 临界区结束
        }

        // 通知一个等待的工作线程，有新任务可执行
        condition.notify_one();

        // 返回 future 对象，调用者可以用它来等待结果或检查异常
        return res;
    }

    // 一个简化的 enqueue 版本，用于不关心返回值的 void() 任务
    void enqueue_void(std::function<void()> task_func) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (stop) {
                throw std::runtime_error(
                    "enqueue_void on stopped TaskExecutor");
            }
            tasks.emplace(std::move(task_func));
        }
        condition.notify_one();
    }

   private:
    // 存储工作线程对象的容器
    std::vector<std::thread> workers;
    // 任务队列，存储待执行的任务 (std::function<void()> 类型)
    std::queue<std::function<void()>> tasks;

    // 互斥锁，用于保护对任务队列的访问
    std::mutex queue_mutex;
    // 条件变量，用于工作线程的等待和唤醒
    std::condition_variable condition;
    // 标志位，指示线程池是否应该停止
    bool stop{false};
};

#endif  // M_THREADPOOL_H
