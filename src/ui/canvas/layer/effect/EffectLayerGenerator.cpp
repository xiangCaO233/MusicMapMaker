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

    // 绘制当前时间字符串
    auto timestr = QString::number(int64_t(
        mapinfo->realTimeInfo.current_time_info.presentation_canvas_time));
    auto timestru32 = timestr.toStdU32String();
    // l->stringMetrics("ComicShannsMono Nerd Font", 16, timestru32);
    auto character_commands =
        l->generateStringCommands("ComicShannsMono Nerd Font", 16, timestru32,
                                  {mapinfo->editorInfo.track_layout.x +
                                       mapinfo->editorInfo.track_layout.z + 8.f,
                                   judgeline_absolute_y - 8.f},
                                  {1.f, 1.f, 0.f, 1.f});

    // buffer.add_PrimitiveCommand(std::move(character_commands));

    // 处理特效绘制
    effect_render_system.update(ecore, mapinfo, l, buffer);

    // 处理音频效果播放
    audio_effect_system.update(ecore, mapinfo);

    // qDebug() << "effect layer done";
}
