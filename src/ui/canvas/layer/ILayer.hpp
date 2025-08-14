#ifndef MMM_ILAYER_HPP
#define MMM_ILAYER_HPP

#include <array>
#include <render/RenderCommand.hpp>
#include <render/Renderer2D.hpp>
#include <string_view>
#include <vector>

enum class LayerType : uint32_t {
    // 背景图层
    BACKGROUND = 0,
    // 时间线图层
    TIMELINE = 1,
    // 物件图层
    NOTE = 2,
    // 特效图层
    EFFECT = 3,
    // 交互图层
    INTERACT = 4,
};

class SharedCanvasInfo;
class Renderer2D;
class ECSCore;

// 图层
class ILayer {
   public:
    using RenderDataBuffer = std::vector<RenderCommand>;
    // 构造ILayer
    ILayer(Renderer2D* renderer, ECSCore* ecore)
        : rendererRef(renderer), core(ecore) {}

    // 析构ILayer
    virtual ~ILayer() = default;

    // 获取图层类型
    LayerType type() const { return layer_type; }

    // 交换前后缓冲区
    void swapBuffers() {
        // 原子翻转索引 (0 -> 1 , 1 -> 0)
        int currentIndex = frontBufferIndex.load(std::memory_order_relaxed);
        int nextIndex = 1 - currentIndex;
        // 使用 acquire-release 内存顺序确保内存可见性
        frontBufferIndex.store(nextIndex, std::memory_order_release);
    }

    // 访问后端缓冲区
    RenderDataBuffer& backbuffer() {
        int backIndex = 1 - frontBufferIndex.load(std::memory_order_relaxed);
        return buffers[backIndex];
    }

    // 访问前端缓冲区
    const RenderDataBuffer& frontbuffer() const {
        int frontIndex = frontBufferIndex.load(std::memory_order_relaxed);
        return buffers[frontIndex];
    }

   protected:
    // 更新信息
    virtual void updateInfo(SharedCanvasInfo* info) = 0;
    void setType(LayerType t) { layer_type = t; }

    // 获取纹理信息
    TextureInfo textureInfo(std::string_view texpath) const {
        if (auto texinfoOption = rendererRef->texture_pool()->get(texpath);
            texinfoOption.has_value()) {
            return texinfoOption.value();
        }
        return {};
    }

   private:
    // 图层类型
    LayerType layer_type;

    // 渲染器引用
    Renderer2D* rendererRef;

    // ecs核心引用
    ECSCore* core;

    // 双缓冲
    // 两个物理缓冲区
    std::array<RenderDataBuffer, 2> buffers;

    // 使用原子整数作为索引
    std::atomic<int> frontBufferIndex;

    friend class LayerManager;
};

#endif  // MMM_ILAYER_HPP
