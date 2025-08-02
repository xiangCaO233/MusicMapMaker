#ifndef MMM_SPECTROGRAMGENERATOR_HPP
#define MMM_SPECTROGRAMGENERATOR_HPP

#include <fftw3.h>
#include <qdebug.h>

#include <atomic>
#include <cmath>
#include <condition_variable>
#include <mutex>
#include <numbers>
#include <queue>
#include <span>
#include <thread>
#include <vector>

template <typename T>
class ThreadSafeQueue {
   public:
    ThreadSafeQueue() = default;
    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

    void push(T new_value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(std::move(new_value));
        m_cond.notify_one();
    }

    // 非阻塞式获取，如果队列为空则立即返回false
    bool try_pop(T& value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty()) {
            return false;
        }
        value = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

   private:
    std::mutex m_mutex;
    std::queue<T> m_queue;
    std::condition_variable m_cond;
};

// 频谱的一列（一个时间帧的结果）定义为 std::vector<float>
using SpectrogramColumn = std::vector<float>;

class SpectrogramGenerator {
   public:
    // 构造SpectrogramGenerator
    explicit SpectrogramGenerator(
        std::shared_ptr<ThreadSafeQueue<SpectrogramColumn>> queue)
        : m_output_queue(queue) {}
    // 析构SpectrogramGenerator
    ~SpectrogramGenerator() {
        stop();  // 确保线程停止

        // 释放FFTW资源
        if (m_plan) fftwf_destroy_plan(m_plan);
        if (m_fftw_in) fftwf_free(m_fftw_in);
        if (m_fftw_out) fftwf_free(m_fftw_out);
    }

    // 禁用拷贝
    SpectrogramGenerator(const SpectrogramGenerator&) = delete;
    SpectrogramGenerator& operator=(const SpectrogramGenerator&) = delete;

    // 启动处理流程
    void start(std::span<const float> audio_data, int windowSize, int hopSize) {
        stop();  // 如果之前在运行，先停止

        m_source_data = audio_data;
        m_windowSize = windowSize;
        m_hopSize = hopSize;

        // 初始化FFTW资源
        // 使用更高性能的实数到复数(r2c)转换
        m_fftw_in = (float*)fftwf_malloc(sizeof(float) * m_windowSize);
        m_fftw_out = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) *
                                                  (m_windowSize / 2 + 1));
        m_plan = fftwf_plan_dft_r2c_1d(m_windowSize, m_fftw_in, m_fftw_out,
                                       FFTW_ESTIMATE);

        // 创建汉宁窗
        m_window.resize(m_windowSize);
        for (int i = 0; i < m_windowSize; ++i) {
            m_window[i] = 0.5f * (1.0f - std::cos(2.0f * std::numbers::pi * i /
                                                  (m_windowSize - 1)));
        }

        // 启动工作线程
        m_is_running = true;
        m_worker_thread =
            std::jthread(&SpectrogramGenerator::processing_thread, this);
    }

    // 停止处理流程
    void stop() {
        m_is_running = false;
        // m_worker_thread 会在析构时自动 join
    }

    int getFrequencyBinCount() const { return m_windowSize / 2 + 1; }

   private:
    // 线程函数
    void processing_thread() {
        size_t num_samples = m_source_data.size();
        if (num_samples == 0) {
            m_is_running = false;
            return;
        }

        for (size_t offset = 0;
             (offset + m_windowSize) <= num_samples && m_is_running;
             offset += m_hopSize) {
            // 1. 分帧和加窗
            for (int i = 0; i < m_windowSize; ++i) {
                m_fftw_in[i] = m_source_data[offset + i] * m_window[i];
            }

            // 2. 执行FFT
            fftwf_execute(m_plan);

            // 3. 计算幅度并转换为dB
            int freq_bins = m_windowSize / 2 + 1;
            SpectrogramColumn column(freq_bins);
            for (int i = 0; i < freq_bins; ++i) {
                float real = m_fftw_out[i][0];
                float imag = m_fftw_out[i][1];
                float magnitude = sqrt(real * real + imag * imag);
                // 加上一个极小值防止log(0)
                column[i] = 20.0f * log10f(magnitude + 1e-9f);
            }

            // 4. 将结果推入队列
            m_output_queue->push(std::move(column));
        }

        qDebug() << "Generator finished processing.";
        m_is_running = false;
    }

    // STFT参数
    int m_windowSize = 0;
    int m_hopSize = 0;

    // FFTW资源
    fftwf_plan m_plan = nullptr;
    float* m_fftw_in = nullptr;
    fftwf_complex* m_fftw_out = nullptr;
    std::vector<float> m_window;

    // 线程管理
    std::jthread m_worker_thread;
    std::atomic<bool> m_is_running = false;
    std::span<const float> m_source_data;

    // 输出队列
    std::shared_ptr<ThreadSafeQueue<SpectrogramColumn>> m_output_queue;
};

#endif  // MMM_SPECTROGRAMGENERATOR_HPP
