#include <layer/MapLayerManager.hpp>

#include "layer/preview/PreviewLayer.hpp"
#include "layer/preview/PreviewLayerGenerator.hpp"

// 析构MapLayerManager
MapLayerManager::~MapLayerManager() = default;

// 更新map
void MapLayerManager::updateMap(MMap* map) {
    mapref = map;
    map->register_editor(editEventQueue);
    map_ecs_core.updateMap(map);
}

// 初始化图层
void MapLayerManager::initializeLayers() {
    // 初始化渲染数据缓冲区
    using enum LayerType;

    auto background_layer =
        layer_data()
            .try_emplace(BACKGROUND, std::make_unique<BackgroundLayer>(
                                         render(), &map_ecs_core))
            .first->second.get();
    auto timeline_layer =
        layer_data()
            .try_emplace(TIMELINE, std::make_unique<TimelineLayer>(
                                       render(), &map_ecs_core))
            .first->second.get();
    auto note_layer = layer_data()
                          .try_emplace(NOTE, std::make_unique<NoteLayer>(
                                                 render(), &map_ecs_core))
                          .first->second.get();
    auto preview_layer =
        layer_data()
            .try_emplace(PREVIEW, std::make_unique<PreviewLayer>(render(),
                                                                 &map_ecs_core))
            .first->second.get();

    auto effect_layer = layer_data()
                            .try_emplace(EFFECT, std::make_unique<EffectLayer>(
                                                     render(), &map_ecs_core))
                            .first->second.get();
    auto interact_layer =
        layer_data()
            .try_emplace(INTERACT, std::make_unique<RealTimeInteractLayer>(
                                       render(), &map_ecs_core))
            .first->second.get();

    // 初始化图层生成器
    auto background_generator =
        layer_generators()
            .try_emplace(BACKGROUND, std::make_unique<BackgroundLayerGenerator>(
                                         this, background_layer, &sync()))
            .first->second.get();

    auto timeline_generator =
        layer_generators()
            .try_emplace(TIMELINE, std::make_unique<TimelineLayerGenerator>(
                                       this, timeline_layer, &sync()))
            .first->second.get();

    auto note_generator =
        layer_generators()
            .try_emplace(NOTE, std::make_unique<NoteLayerGenerator>(
                                   this, note_layer, &sync(), get_tool_system(),
                                   get_tool_interaction_state()))
            .first->second.get();

    auto preview_generator =
        layer_generators()
            .try_emplace(PREVIEW,
                         std::make_unique<PreviewLayerGenerator>(
                             this, preview_layer, &sync(), get_tool_system(),
                             get_tool_interaction_state()))
            .first->second.get();

    auto effect_generator =
        layer_generators()
            .try_emplace(EFFECT, std::make_unique<EffectLayerGenerator>(
                                     this, effect_layer, &sync()))
            .first->second.get();

    auto interact_generator =
        layer_generators()
            .try_emplace(INTERACT, std::make_unique<InteractLayerGenerator>(
                                       this, interact_layer, &sync(),
                                       get_tool_interaction_state()))
            .first->second.get();

    // 启动图层生成器
    startGenerator(BACKGROUND, background_generator);
    startGenerator(TIMELINE, timeline_generator);
    startGenerator(NOTE, note_generator);
    startGenerator(PREVIEW, preview_generator);
    startGenerator(EFFECT, effect_generator);
    startGenerator(INTERACT, interact_generator);
}
