#ifndef MMM_IRENDERER_HPP
#define MMM_IRENDERER_HPP

#include <qpaintdevice.h>

#include <QPainter>
#include <audio/control/ProcessChain.hpp>
#include <ice/core/IAudioNode.hpp>
#include <ice/manage/AudioTrack.hpp>
#include <memory>

// 所有渲染器的基类接口
class IRenderer {
   public:
    virtual ~IRenderer() = default;

    // 根据当前状态处理一段音频数据
    virtual void process(const ProcessChain* chain,
                         const std::shared_ptr<ice::AudioTrack>& trackInfo,
                         size_t startFrame, size_t numFrames) = 0;

    // 核心绘制函数
    virtual void paint(QPainter* painter, QRect rect) = 0;
};

#endif  // MMM_IRENDERER_HPP
