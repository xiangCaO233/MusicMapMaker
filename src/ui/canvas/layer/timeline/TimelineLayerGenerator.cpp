#include <log/colorful-log.h>

#include <QDebug>
#include <ecs/component/TransformComponents.hpp>
#include <ecs/system/time2pixel/maintrack/LinearTimeConverter.hpp>
#include <info/MapCanvasInfo.hpp>
#include <layer/MapLayerManager.hpp>
#include <layer/timeline/TimelineLayerGenerator.hpp>

// 析构TimelineLayerGenerator
TimelineLayerGenerator::~TimelineLayerGenerator() {
    XINFO("时间线图层生成线程释放");
}

// 生成交互层的数据
void TimelineLayerGenerator::generateLayer(LayerManager* manager,
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
            mapinfo->editorInfo.map->base_metadata().preference_bpm);

    // 根据设定的坐标系 (Y=0在底部)，计算判定线的绝对像素位置。
    // 如果 judgeline_pos = 0.2f，意味着判定线在从下往上20%的高度。
    const auto judgeline_absolute_y = mapinfo->baseInfo.canvasSize.height() *
                                      mapinfo->editorInfo.judgeline_pos;

    // 计算屏幕顶部和底部到判定线的“相对像素距离”。
    // 这些相对值将作为 converter 的输入。
    // 正值代表“未来”方向（在屏幕上是向上的）。
    // 负值代表“过去”方向（在屏幕上是向下的）。
    const auto pixel_y_top =
        mapinfo->baseInfo.canvasSize.height() - judgeline_absolute_y;
    const auto pixel_y_bottom = 0.0f - judgeline_absolute_y;

    const auto& time =
        mapinfo->realTimeInfo.current_time_info.presentation_canvas_time;
    // 使用转换器计算时间边界
    const auto time_at_top = converter->distanceToTime(pixel_y_top, time);
    const auto time_at_bottom = converter->distanceToTime(pixel_y_bottom, time);

    // 应用预加载缓冲
    const auto query_start_time =
        time_at_bottom - mapinfo->baseInfo.view_timeMargin;
    const auto query_end_time = time_at_top + mapinfo->baseInfo.view_timeMargin;
    // 使用转换器计算Y坐标
    const float start_y =
        converter->timeToPixel(query_start_time, time, mapinfo);
    const float end_y = converter->timeToPixel(query_end_time, time, mapinfo);

    // 绘制读取时间区间线(红色)
    PrimitiveCommand start_cmd;
    start_cmd.cmdType = CommandType::PRIMITIVE;
    start_cmd.baseInfo.pos = {0, start_y};
    start_cmd.baseInfo.size = {mapinfo->baseInfo.canvasSize.width(), 8};
    start_cmd.baseInfo.color = {1, 0, 0, 1};
    start_cmd.primitive = PrimitiveType::QUAD;
    PrimitiveCommand end_cmd;
    end_cmd.cmdType = CommandType::PRIMITIVE;
    end_cmd.baseInfo.pos = {0, end_y};
    end_cmd.baseInfo.size = {mapinfo->baseInfo.canvasSize.width(), 8};
    end_cmd.baseInfo.color = {1, 0, 0, 1};
    end_cmd.primitive = PrimitiveType::QUAD;

    buffer.add_PrimitiveCommand(start_cmd);
    buffer.add_PrimitiveCommand(end_cmd);

    // 生成时间线(拍线/识别分拍/小节线)
    timeline_system.update(ecore, mapinfo, *converter, l, buffer);

    // qDebug() << "timeline layer done";
}
