#ifndef M_RENDERBUFFER_H
#define M_RENDERBUFFER_H

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>

#include "texture/Texture.h"

enum class RenderType : uint8_t {
    MBG = 0,
    MORBITRS = 1,
    MBEAT = 2,
    MEFFECTS = 3,
    MHITOBJECT = 4,
    MPREVIEW = 5,
    MSELECTION = 6,
    MINFO = 7,
    MTOPBAR = 8,
};

enum class FunctionType {
    MRECT,
    MROUNDRECT,
    MELLIPSE,
    MLINE,
    MTEXT,
};

struct RendererManagerSettings {
    // 纹理特效
    TextureEffect texture_effect{TextureEffect::NONE};
    // 纹理对齐模式
    TextureAlignMode texture_alignmode{TextureAlignMode::ALIGN_TO_CENTER};
    // 纹理填充模式
    TextureFillMode texture_fillmode{TextureFillMode::SCALLING_AND_TILE};
    // 纹理补充模式
    TextureComplementMode texture_complementmode{
        TextureComplementMode::REPEAT_TEXTURE};
};

struct RenderParams {
    FunctionType func_type;
    RendererManagerSettings render_settings;
    unsigned char r{0};
    unsigned char g{0};
    unsigned char b{0};
    unsigned char a{255};
    enum class Type { RECT, LINE } type;
    union {
        // 矩形表示
        struct {
            float xpos, ypos, width, height;
            double radius;
        };
        // 线段表示
        struct {
            float x1, y1, x2, y2;
            double line_width;
        };
    };
    double rotation{.0};
    bool is_volatile;
    std::shared_ptr<TextureInstace> texture{nullptr};
    std::u32string str{U""};
    const char* font_family;
};

// 单帧的图层数据集合
struct BufferWrapper {
    std::queue<std::vector<RenderParams>> bg_datas;
    std::queue<std::vector<RenderParams>> orbits_datas;
    std::queue<std::vector<RenderParams>> beats_datas;
    std::queue<std::vector<RenderParams>> effects_datas;
    std::queue<std::vector<RenderParams>> hitobjects_datas;
    std::queue<std::vector<RenderParams>> preview_datas;
    std::queue<std::vector<RenderParams>> selection_datas;
    std::queue<std::vector<RenderParams>> info_datas;
    std::queue<std::vector<RenderParams>> topbar_datas;

    // 清空所有队列
    void clear_all_queues() {
        bg_datas = {};
        orbits_datas = {};
        beats_datas = {};
        effects_datas = {};
        hitobjects_datas = {};
        preview_datas = {};
        selection_datas = {};
        info_datas = {};
        topbar_datas = {};
    }
};

// --- 双缓冲管理器类 ---
class DoubleBufferManager {
   public:
    DoubleBufferManager()
        // 初始时，buffer1 是前端
        : front_buffer_ptr_(&buffer1_),
          // buffer2 是后端
          back_buffer_ptr_(&buffer2_),
          back_buffer_ready_flag_(false),
          // 初始允许计算线程开始填充
          front_buffer_free_flag_(true) {}

    // 计算线程调用此方法来获取后端缓冲区进行写入
    // 返回的指针在 submit_back_buffer() 调用之前有效且独占
    BufferWrapper* acquire_back_buffer_for_writing(
        const std::atomic<bool>& should_exit) {
        std::unique_lock<std::mutex> lock(mutex_);

        // 等待前端缓冲区被渲染线程处理完毕 (front_buffer_free_flag_ 为 true)
        // 或者线程需要退出
        cv_calc_can_fill_.wait(lock, [&] {
            return front_buffer_free_flag_ ||
                   should_exit.load(std::memory_order_relaxed);
        });

        if (should_exit.load(std::memory_order_relaxed)) {
            // 如果需要退出，返回nullptr
            return nullptr;
        }

        // 标记此缓冲区 (当前是 back_buffer_ptr_) 正在被填充
        front_buffer_free_flag_ = false;

        // 清空将要写入的后端缓冲区 (在独占访问时进行)
        back_buffer_ptr_->clear_all_queues();

        // 返回指向后端缓冲区的指针
        return back_buffer_ptr_;
    }

    // 计算线程调用此方法来提交已填充的后端缓冲区
    void submit_back_buffer_and_notify_render() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            // 标记后端缓冲区已准备好
            back_buffer_ready_flag_ = true;
        }
        // 通知渲染线程
        cv_render_can_draw_.notify_one();
    }

    // 渲染线程调用此方法来获取前端缓冲区进行读取
    // 返回的指针在 release_front_buffer_after_reading() 调用之前有效且独占
    BufferWrapper* acquire_front_buffer_for_reading(
        const std::atomic<bool>& should_exit) {
        std::unique_lock<std::mutex> lock(mutex_);

        // 等待后端缓冲区填充完毕 (back_buffer_ready_flag_ 为 true)
        // 或者线程需要退出
        cv_render_can_draw_.wait(lock, [&] {
            return back_buffer_ready_flag_ ||
                   should_exit.load(std::memory_order_relaxed);
        });

        if (should_exit.load(std::memory_order_relaxed)) {
            // 如果需要退出，返回nullptr
            return nullptr;
        }

        // 标记此缓冲区 (当前是 back_buffer_ptr_) 即将被渲染
        back_buffer_ready_flag_ = false;

        // 执行交换，使填充好的后端缓冲区成为新的前端缓冲区
        std::swap(front_buffer_ptr_, back_buffer_ptr_);

        // 返回指向新的前端缓冲区的指针
        return front_buffer_ptr_;
    }

    // 渲染线程调用此方法来表示已完成对前端缓冲区的读取
    void release_front_buffer_and_notify_calc() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            // 标记旧的前端缓冲区 (现在是后端了) 空闲
            front_buffer_free_flag_ = true;
        }
        // 通知计算线程
        cv_calc_can_fill_.notify_one();
    }

    // 用于在程序退出时唤醒可能在等待的线程
    void notify_all_for_exit() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            // 尽管 should_exit 会被检查，但为了确保它们从 wait 中醒来，还是
            // notify
        }
        cv_calc_can_fill_.notify_all();
        cv_render_can_draw_.notify_all();
    }

   private:
    // 实际的缓冲区实例1
    BufferWrapper buffer1_;
    // 实际的缓冲区实例2
    BufferWrapper buffer2_;

    // 指向当前的前端缓冲区
    BufferWrapper* front_buffer_ptr_;
    // 指向当前的后端缓冲区
    BufferWrapper* back_buffer_ptr_;

    // 保护所有共享状态和指针交换
    std::mutex mutex_;
    // 计算线程在此等待
    std::condition_variable cv_calc_can_fill_;
    // 渲染线程在此等待
    std::condition_variable cv_render_can_draw_;

    // 状态标志
    bool back_buffer_ready_flag_;
    bool front_buffer_free_flag_;
};

#endif  // M_RENDERBUFFER_H
