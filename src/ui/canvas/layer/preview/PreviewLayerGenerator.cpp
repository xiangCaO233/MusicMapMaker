#include <QDebug>
#include <ecs/component/TransformComponents.hpp>
#include <info/MapCanvasInfo.hpp>
#include <layer/MapLayerManager.hpp>
#include <layer/preview/PreviewLayer.hpp>
#include <layer/preview/PreviewLayerGenerator.hpp>
#include <unordered_map>

// 析构PreviewLayerGenerator
PreviewLayerGenerator::~PreviewLayerGenerator() {
    qDebug() << "预览图层生成线程释放";
}

// 生成物件层的数据
void PreviewLayerGenerator::generateLayer(LayerManager* manager,
                                          RenderDataBuffer& buffer) {
    // 数据准备
    auto maplayer_manager = static_cast<MapLayerManager*>(manager);
    auto map = maplayer_manager->map();
    if (!map) return;
    auto l = layer<NoteLayer>();
    auto mapinfo = static_cast<MapCanvasInfo*>(l->info());
    auto& ecore = maplayer_manager->core();
    // 从管理器获取时间转换器
    auto& converter =
        maplayer_manager->get_time_converter_manager()->getConverter(
            map->timing_set(), mapinfo->baseInfo,
            mapinfo->editorInfo.scrollInfo, mapinfo,
            mapinfo->editorInfo.map->base_metadata().preference_bpm);

    auto& main_track_layout = mapinfo->editorInfo.track_layout;
    // 移动轨道布局到预览区
    auto xpos = main_track_layout.x + main_track_layout.z;
    auto preview_track_layout =
        glm::vec4{xpos, 0.f, mapinfo->baseInfo.canvasSize.width() - xpos,
                  mapinfo->baseInfo.canvasSize.height()};
    // 绘制预览区遮罩
    PrimitiveCommand previewAreaMaskCmd;
    previewAreaMaskCmd.cmdType = CommandType::PRIMITIVE;
    previewAreaMaskCmd.primitive = PrimitiveType::QUAD;
    previewAreaMaskCmd.baseInfo.pos = {preview_track_layout.x,
                                       preview_track_layout.y};
    previewAreaMaskCmd.baseInfo.size = {preview_track_layout.z,
                                        preview_track_layout.w};
    previewAreaMaskCmd.baseInfo.color = {0.2f, 0.2f, 0.2f, 0.4f};
    buffer.add_PrimitiveCommand(previewAreaMaskCmd);

    // 生成预览物件网格
    std::unordered_map<entt::entity, GeneratedMesh> preview_meshs;
    mesh_system.update(ecore.ecs_registry(), mapinfo, converter,
                       tool_interaction_state, preview_meshs, true);

    // 渲染预览区可见物件
    normalRender_system.update(ecore.ecs_registry(), preview_meshs, mapinfo,
                               converter, l, buffer);

    // qDebug() << "preview layer done";
}
