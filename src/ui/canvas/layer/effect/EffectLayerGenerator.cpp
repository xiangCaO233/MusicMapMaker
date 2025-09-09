#include <QDebug>
#include <layer/MapLayerManager.hpp>
#include <layer/effect/EffectLayerGenerator.hpp>

// 析构EffectLayerGenerator
EffectLayerGenerator::~EffectLayerGenerator() {
    qDebug() << "效果图层生成线程释放";
}

// 生成图层
void EffectLayerGenerator::generateLayer(LayerManager* manager,
                                         RenderDataBuffer& buffer) {
    // 数据准备
    auto maplayer_manager = static_cast<MapLayerManager*>(manager);
    auto map = maplayer_manager->map();
    if (!map) return;
    auto l = layer<NoteLayer>();
    auto mapinfo = static_cast<MapCanvasInfo*>(l->info());
    auto& ecore = maplayer_manager->core();
    const auto judgeline_absolute_y = mapinfo->baseInfo.canvasSize.height() *
                                      (1.f - mapinfo->baseInfo.judgeline_pos);
    auto skin = mapinfo->editorInfo.skin;
    // 绘制判定线
    // PrimitiveCommand judgeline_cmd;
    // judgeline_cmd.cmdType = CommandType::PRIMITIVE;
    // judgeline_cmd.baseInfo.pos = {mapinfo->editorInfo.track_layout.x,
    //                               judgeline_absolute_y};
    // judgeline_cmd.baseInfo.size = {mapinfo->editorInfo.track_layout.z, 4};
    // judgeline_cmd.baseInfo.color = {0, 1, 1, 1};
    // judgeline_cmd.primitive = PrimitiveType::QUAD;
    // buffer.add_PrimitiveCommand(judgeline_cmd);

    // 绘制当前时间字符串
    auto timestr = QString::number(
        uint32_t(mapinfo->realTimeInfo.current_time_info.logic_canvas_time));
    auto timestru32 = timestr.toStdU32String();

    uint32_t xoffset{0};
    uint32_t yoffset{0};

    for (const auto& character : timestru32) {
        // 获取字符纹理信息
        auto fontoption = l->get("ComicShannsMono Nerd Font", 16, character);
        if (fontoption.has_value()) {
            auto& charInfo = fontoption.value();
            auto& charTexture = charInfo.character_texinfo;

            // 计算当前字符应该处于的位置
            glm::vec2 charpos = {mapinfo->editorInfo.track_layout.x +
                                     mapinfo->editorInfo.track_layout.z + 8.f,
                                 judgeline_absolute_y};
            charpos.x += xoffset;
            charpos.y -= (charInfo.bearing.y);
            charpos.y += 8;
            // 提交渲染指令
            PrimitiveCommand charcommand{
                {CommandType::PRIMITIVE,
                 {charTexture, TexAlignMode::CENTER, TexScaleMode::CHARACTER}},
                charpos,
                charTexture.origin_size,
                0.f,
                {1.f, 1.f, 0.f, 1.f},
                true,
                {charTexture.uv_offset},
                PrimitiveType::QUAD};
            buffer.add_PrimitiveCommand(charcommand);

            xoffset += charInfo.xadvance / 64;
        }
    }

    // 处理特效绘制
    effect_render_system.update(ecore, mapinfo, l, buffer);

    // 处理音频效果播放
    audio_effect_system.update(ecore, mapinfo);

    // qDebug() << "effect layer done";
}
