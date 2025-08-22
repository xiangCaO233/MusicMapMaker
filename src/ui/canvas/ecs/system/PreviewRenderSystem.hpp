#ifndef MMM_PREVIEWRENDERSYSTEM_HPP
#define MMM_PREVIEWRENDERSYSTEM_HPP

#include <ecs/ECSCore.hpp>
#include <ecs/component/CoreComponents.hpp>
#include <ecs/component/NoteComponents.hpp>
#include <ecs/component/TransformComponents.hpp>
#include <ecs/system/TimePixelConverter.hpp>
#include <info/MapCanvasInfo.hpp>
#include <layer/ILayer.hpp>

// 渲染预览虚影物件和即将删除的物件
class PreviewRenderSystem {
   public:
    void update(ECSCore& core, const MapCanvasInfo* info,
                const TimePixelConverter& converter,
                ILayer::RenderDataBuffer& buffer) const {}
};

#endif  // MMM_PREVIEWRENDERSYSTEM_HPP
