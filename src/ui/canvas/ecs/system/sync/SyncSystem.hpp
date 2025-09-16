#ifndef MMM_SYNCSYSTEM_HPP
#define MMM_SYNCSYSTEM_HPP

#include <ecs/ECSCore.hpp>
#include <ecs/component/CoreComponents.hpp>
#include <ecs/component/EffectComponents.hpp>
#include <ecs/component/NoteComponents.hpp>
#include <ecs/component/RelationComponents.hpp>
#include <ecs/component/TimeLineComponents.hpp>
#include <ecs/component/TimingComponents.hpp>
#include <ecs/component/TransformComponents.hpp>
#include <ecs/system/TimePixelConverter.hpp>
#include <entt.hpp>
#include <info/MapCanvasInfo.hpp>
#include <mmm/DataStructures.hpp>
#include <mmm/map/MMap.hpp>
#include <mmm/obj/Hold.hpp>
#include <mmm/obj/rm/Composite.hpp>
#include <mmm/obj/rm/Slide.hpp>

class MapLayerManager;

class SyncSystem {
   public:
    // 同步工具交互
    void updateToolInteractions(ECSCore& core, const MapCanvasInfo* info,
                                MapLayerManager* layer_manager) const;

    // 同步特效实体
    void updateEffects(ECSCore& core, const NoteCollection& notes,
                       const NoteIDManager& uuidManager,
                       const MapCanvasInfo* info,
                       const TimePixelConverter& converter) const;

    // 同步物件/拍/时间点实体
    void updateEntities(ECSCore& core, const NoteCollection& notes,
                        const NoteIDManager& uuidManager,
                        MapLayerManager* layer_manager,
                        const TimingMap& timings,
                        const BeatTimeline& beatTimeLine,
                        const BeatInfo& beatInfo, const MapCanvasInfo* info,
                        const TimePixelConverter& converter) const;
};

#endif  // MMM_SYNCSYSTEM_HPP
