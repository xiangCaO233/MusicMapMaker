#include <QDebug>
#include <info/MapCanvasInfo.hpp>
#include <info/SharedCanvasInfo.hpp>
#include <layer/background/BackgroundLayer.hpp>
#include <layer/background/BackgroundLayerGenerator.hpp>
#include <map/skin/MSkin.hpp>
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
        const float single_track_width = all_tracks_rect.z / float(track_count);

        auto background_image_path = info->mapInfo.cover_path;
        auto darken = info->mapInfo.darken;
        auto alpha = info->mapInfo.alpha;
        auto texinfo = bglayer->textureInfo(background_image_path);

        // 绘制背景图片
        if (!background_image_path.empty()) {
            PrimitiveCommand cmd;
            cmd.cmdType = CommandType::PRIMITIVE;
            cmd.baseInfo = {{0.f, 0.f}, canvas_size};
            cmd.baseInfo.color = {darken, darken, darken, alpha};
            cmd.texturesInfo.texture = texinfo;
            cmd.primitive = PrimitiveType::QUAD;
            buffer.add_PrimitiveCommand(cmd);
        }

        // 绘制轨道纹理
        auto oribit_bg_texture = info->editorInfo.skin->get_orbit_bg_texture();
        auto oribit_judge_texture =
            info->editorInfo.skin->get_orbit_judge_texture();
        for (int i{0}; i < track_count; ++i) {
            PrimitiveCommand cmd;
            cmd.cmdType = CommandType::PRIMITIVE;
            cmd.primitive = PrimitiveType::QUAD;
            cmd.baseInfo = {{all_tracks_rect.x + i * single_track_width, 0.f},
                            {single_track_width, canvas_size.y}};
            cmd.texturesInfo.texture = oribit_bg_texture;
            cmd.texturesInfo.tscale = TexScaleMode::TILE_BASEWIDTH_REPEAT;
            buffer.add_PrimitiveCommand(cmd);

            PrimitiveCommand jcmd;
            jcmd.cmdType = CommandType::PRIMITIVE;
            jcmd.primitive = PrimitiveType::QUAD;
            glm::vec2 judgeline_size{single_track_width,
                                     single_track_width /
                                         oribit_judge_texture.origin_size.x *
                                         oribit_judge_texture.origin_size.y};
            judgeline_size *= 1.25f;
            jcmd.baseInfo = {
                {all_tracks_rect.x + (float(i) + .5f) * single_track_width -
                     judgeline_size.x / 2.f,
                 (1.f - info->baseInfo.judgeline_pos) * canvas_size.y -
                     judgeline_size.y / 2.f},
                judgeline_size};
            jcmd.texturesInfo.texture = oribit_judge_texture;
            buffer.add_PrimitiveCommand(jcmd);
        }
    }
    // qDebug() << "bg layer done";
}
