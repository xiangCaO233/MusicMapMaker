#include <QDebug>
#include <info/MapCanvasInfo.hpp>
#include <info/SharedCanvasInfo.hpp>
#include <layer/background/BackgroundLayer.hpp>
#include <layer/background/BackgroundLayerGenerator.hpp>
#include <render/command/RenderCommand.hpp>

// 析构BackgroundLayerGenerator
BackgroundLayerGenerator::~BackgroundLayerGenerator() {
    qDebug() << "背景图层生成线程释放";
}

// 生成图层
void BackgroundLayerGenerator::generateLayer(LayerManager* manager,
                                             RenderDataBuffer& buffer) {
    auto bglayer = layer<BackgroundLayer>();
    auto info = static_cast<MapCanvasInfo*>(bglayer->info());
    if (info->editorInfo.map) {
        glm::vec2 canvas_size = {info->baseInfo.canvasSize.width(),
                                 info->baseInfo.canvasSize.height()};

        // 获取轨道布局信息
        const glm::vec4& all_tracks_rect = info->editorInfo.track_layout;
        const int track_count =
            info->editorInfo.map->base_metadata().track_count;
        if (track_count == 0) return;
        // const float single_track_width = all_tracks_rect.z /
        // float(track_count);

        auto background_image_path = info->mapInfo.cover_path;
        auto darken = info->mapInfo.darken;
        auto alpha = info->mapInfo.alpha;
        auto texinfo = bglayer->textureInfo(background_image_path);

        // 绘制背景图片
        if (!background_image_path.empty()) {
            QuadCommand cmd;
            cmd.cmdType = CommandType::QUAD;
            cmd.baseInfo = {{0.f, 0.f}, canvas_size, 0.f};
            cmd.baseInfo.color = {darken, darken, darken, alpha};
            cmd.texturesInfo.texture = texinfo;
            buffer.add_QuadCommand(cmd);
        }

        // 绘制轨道边界线
        QuadCommand track_edge_leftcmd;
        track_edge_leftcmd.cmdType = CommandType::QUAD;
        track_edge_leftcmd.baseInfo.pos = {all_tracks_rect.x - 3, 0};
        track_edge_leftcmd.baseInfo.size = {6,
                                            info->baseInfo.canvasSize.height()};
        track_edge_leftcmd.baseInfo.color = {0, 1, 1, 1};
        QuadCommand track_edge_rightcmd;
        track_edge_rightcmd.cmdType = CommandType::QUAD;
        track_edge_rightcmd.baseInfo.pos = {
            all_tracks_rect.x + all_tracks_rect.z - 3, 0};
        track_edge_rightcmd.baseInfo.size = {
            6, info->baseInfo.canvasSize.height()};
        track_edge_rightcmd.baseInfo.color = {0, 1, 1, 1};
        buffer.add_QuadCommand(track_edge_leftcmd);
        buffer.add_QuadCommand(track_edge_rightcmd);
    }
    // qDebug() << "bg layer done";
}
