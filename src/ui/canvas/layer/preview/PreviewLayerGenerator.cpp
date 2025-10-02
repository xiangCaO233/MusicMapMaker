#include <log/colorful-log.h>

#include <QDebug>
#include <ecs/component/TransformComponents.hpp>
#include <info/MapCanvasInfo.hpp>
#include <layer/MapLayerManager.hpp>
#include <layer/preview/PreviewLayer.hpp>
#include <layer/preview/PreviewLayerGenerator.hpp>
#include <unordered_map>

// 析构PreviewLayerGenerator
PreviewLayerGenerator::~PreviewLayerGenerator() {
    XINFO("预览图层生成线程释放");
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
    auto converter =
        maplayer_manager->get_time_converter_manager()->getConverter(
            map->timing_set(), mapinfo->baseInfo,
            mapinfo->editorInfo.scrollInfo, mapinfo,
            mapinfo->editorInfo.map->base_metadata().preference_bpm, true);

    auto& main_track_layout = mapinfo->editorInfo.track_layout;
    // 移动轨道布局到预览区
    auto xpos = main_track_layout.x + main_track_layout.z;
    auto preview_track_layout =
        glm::vec4{xpos, 0.f, mapinfo->baseInfo.canvasSize.width() - xpos,
                  mapinfo->baseInfo.canvasSize.height()};

    if (!tool_interaction_state->isDraggingGlobalPreview()) {
        // 正在全局拖动-不绘制物件和时间线
        // 绘制时间线和timing
        timeline_system.update(ecore, mapinfo, *converter, l,
                               maplayer_manager->get_tool_interaction_state(),
                               buffer, true);

        // 生成预览物件网格
        std::unordered_map<entt::entity, GeneratedMesh> preview_meshs;
        mesh_system.update(ecore.ecs_registry(), mapinfo, *converter,
                           tool_interaction_state, preview_meshs, true);

        // 渲染预览区可见物件
        render_system.update(ecore.ecs_registry(), preview_meshs, mapinfo,
                             *converter, l, buffer, true);
    }

    // 然后绘制预览区遮罩
    PrimitiveCommand previewAreaMaskCmd;
    previewAreaMaskCmd.cmdType = CommandType::PRIMITIVE;
    previewAreaMaskCmd.primitive = PrimitiveType::QUAD;
    previewAreaMaskCmd.baseInfo.pos = {preview_track_layout.x,
                                       preview_track_layout.y};
    previewAreaMaskCmd.baseInfo.size = {preview_track_layout.z,
                                        preview_track_layout.w};
    previewAreaMaskCmd.baseInfo.color = {0.2f, 0.2f, 0.2f, 0.4f};
    buffer.add_PrimitiveCommand(previewAreaMaskCmd);

    if (tool_interaction_state->isDraggingGlobalPreview()) {
        // 正在全局拖动-不绘制主轨道位置和判定线
        // 绘制当前时间比例的进度
        auto progress_height =
            preview_track_layout.w *
            (mapinfo->realTimeInfo.current_time_info.presentation_canvas_time /
             double(mapinfo->editorInfo.map->base_metadata().map_length));
        // 绘制进度遮罩
        PrimitiveCommand previewProgressMaskCmd;
        previewProgressMaskCmd.cmdType = CommandType::PRIMITIVE;
        previewProgressMaskCmd.primitive = PrimitiveType::QUAD;
        previewProgressMaskCmd.baseInfo.pos = {
            preview_track_layout.x, preview_track_layout.w - progress_height};
        previewProgressMaskCmd.baseInfo.size = {preview_track_layout.z,
                                                progress_height};
        previewProgressMaskCmd.baseInfo.color = {0.9f, .9f, .9f, 0.6f};
        buffer.add_PrimitiveCommand(previewProgressMaskCmd);

    } else {
        // 绘制预览区中的位置遮罩
        const auto& editor_info = mapinfo->editorInfo;
        const auto& base_info = mapinfo->baseInfo;
        const float canvas_height = base_info.canvasSize.height();
        auto pconfig = editor_info.map->project()->cfg();

        // 预览区相对主轨道的倍率
        auto maintrackpos_inpreview_area_ratio =
            editor_info.previewAreaInfo.areaRatio;
        // 主轨道在预览区中的高度
        auto maintrack_size_inpreview =
            canvas_height / maintrackpos_inpreview_area_ratio;
        // 主轨道中心在预览区中的倍率
        auto maintrackpos_inpreview_area =
            editor_info.previewAreaInfo.mainAreaPos;
        // 主轨道中心在预览区中的位置
        auto maintrack_center_inpreview =
            maintrackpos_inpreview_area * canvas_height;
        // 主轨道顶部在预览区中的位置
        auto maintrack_top_inpreview =
            maintrack_center_inpreview - maintrack_size_inpreview / 2.f;
        // 主轨道判定线在预览区中的位置
        auto judgeline_pos_in_previewarea =
            maintrack_top_inpreview +
            pconfig->canvas_config.judgeline_pos * maintrack_size_inpreview;

        PrimitiveCommand previewAreaMainTrackMaskCmd;
        previewAreaMainTrackMaskCmd.cmdType = CommandType::PRIMITIVE;
        previewAreaMainTrackMaskCmd.primitive = PrimitiveType::QUAD;
        previewAreaMainTrackMaskCmd.baseInfo.pos = {preview_track_layout.x,
                                                    maintrack_top_inpreview};
        previewAreaMainTrackMaskCmd.baseInfo.size = {preview_track_layout.z,
                                                     maintrack_size_inpreview};
        previewAreaMainTrackMaskCmd.baseInfo.color = {0.8f, 0.8f, 0.8f, 0.2f};
        buffer.add_PrimitiveCommand(previewAreaMainTrackMaskCmd);

        // 绘制预览区判定线
        PrimitiveCommand previewAreaMainTrackJudgelineCmd;
        previewAreaMainTrackJudgelineCmd.cmdType = CommandType::PRIMITIVE;
        previewAreaMainTrackJudgelineCmd.primitive = PrimitiveType::QUAD;
        previewAreaMainTrackJudgelineCmd.baseInfo.pos = {
            preview_track_layout.x, judgeline_pos_in_previewarea - 1.f};
        previewAreaMainTrackJudgelineCmd.baseInfo.size = {
            preview_track_layout.z, 2.f};
        previewAreaMainTrackJudgelineCmd.baseInfo.color = {0.f, 1.f, 1.f, 0.8f};
        buffer.add_PrimitiveCommand(previewAreaMainTrackJudgelineCmd);
    }

    // qDebug() << "preview layer done";
}
