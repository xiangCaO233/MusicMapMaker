#include "TimeInfoGenerator.h"

#include <QString>
#include <algorithm>
#include <memory>

#include "../../../mmm/MapWorkProject.h"
#include "../../../util/mutil.h"
#include "../MapWorkspaceCanvas.h"
#include "../RenderBuffer.hpp"
#include "../editor/MapEditor.h"

// 构造TimeInfoGenerator
TimeInfoGenerator::TimeInfoGenerator(std::shared_ptr<MapEditor> &editor)
    : editor_ref(editor) {}

// 析构TimeInfoGenerator
TimeInfoGenerator::~TimeInfoGenerator() = default;

// 生成信息
void TimeInfoGenerator::generate(BufferWrapper *bufferwrapper) {
    // 分隔线
    editor_ref->canvas_ref->renderer_manager->addLine(
        QPointF(editor_ref->cstatus.canvas_size.width() *
                        editor_ref->csettings.infoarea_width_scale -
                    2,
                0),
        QPointF(editor_ref->cstatus.canvas_size.width() *
                        editor_ref->csettings.infoarea_width_scale -
                    2,
                editor_ref->cstatus.canvas_size.height()),
        4, nullptr, QColor(255, 182, 193, 235), false);

    // 标记timing
    draw_timing_points(bufferwrapper);
}

// 绘制时间点
void TimeInfoGenerator::draw_timing_points(BufferWrapper *bufferwrapper) {
    if (!editor_ref->canvas_ref->working_map) return;
    auto &info_params_list = bufferwrapper->info_datas.emplace();

    std::string bpm_prefix = "bpm:";
    std::vector<std::vector<std::shared_ptr<Timing>> *> timingss_in_area;

    // 查询区间内的timings
    editor_ref->canvas_ref->working_map->query_timing_in_range(
        timingss_in_area, editor_ref->ebuffer.current_time_area_start,
        editor_ref->ebuffer.current_time_area_end);

    for (const auto &timings : timingss_in_area) {
        // 实际在此时间点显示的timing的bpm值
        double display_bpm_value{0};
        // 是否显示变速值
        bool show_speed{false};
        // 若显示变速值参考的timing
        std::shared_ptr<Timing> speed_timing_ref{nullptr};

        // 根据此同时间戳timing表大小确定是否存在即时变速
        // 预处理同时间戳timing表
        if (timings->size() >= 2) {
            // 包含同时间点timing
            if (timings->at(0)->basebpm == timings->at(1)->basebpm) {
                display_bpm_value = timings->at(0)->basebpm;
            }

            // 检查是否存在变速timing
            for (const auto &timing : *timings) {
                if (!timing->is_base_timing) {
                    show_speed = true;
                    speed_timing_ref = timing;
                }
            }
        } else {
            // 单独timing-可能是参考bpm也可能是变速
            display_bpm_value = timings->at(0)->basebpm;
            if (!timings->at(0)->is_base_timing) {
                show_speed = true;
                speed_timing_ref = timings->at(0);
            }
        }

        // 格式化显示的字符串参考-bpm
        std::string preference_bpm_str =
            "bpm:" + QString::number(display_bpm_value, 'f', 2).toStdString();
        std::string speed_str = "";

        if (show_speed) {
            speed_str =
                "speed:" +
                QString::number(speed_timing_ref->bpm, 'f', 2).toStdString();
        }

        std::u32string u32absbpmstr = mutil::cu32(preference_bpm_str);
        std::u32string u32speedstr = mutil::cu32(speed_str);

        // 计算字符串metrics
        double prebpmstr_width{0};
        double prebpmstr_height{0};

        // 参考bpm
        for (const auto &character : u32absbpmstr) {
            auto charsize =
                editor_ref->canvas_ref->renderer_manager->get_character_size(
                    editor_ref->canvas_ref->skin.font_family,
                    editor_ref->canvas_ref->skin.timeinfo_font_size, character);
            prebpmstr_width += charsize.width();

            if (charsize.height() > prebpmstr_height) {
                prebpmstr_height = charsize.height();
            }
        }

        double speedstr_width{0};
        double speedstr_height{0};
        // 速度
        for (const auto &character : u32speedstr) {
            auto charsize =
                editor_ref->canvas_ref->renderer_manager->get_character_size(
                    editor_ref->canvas_ref->skin.font_family,
                    editor_ref->canvas_ref->skin.timeinfo_font_size, character);
            speedstr_width += charsize.width();

            if (charsize.height() > speedstr_height) {
                speedstr_height = charsize.height();
            }
        }

        // 全部字符串的尺寸
        QSizeF strs_size(std::max(prebpmstr_width, speedstr_width),
                         prebpmstr_height + speedstr_height);

        // 框的尺寸
        QSizeF inner_bound_size(
            strs_size.width() + editor_ref->canvas_ref->skin.timeinfo_font_size,
            strs_size.height() +
                editor_ref->canvas_ref->skin.timeinfo_font_size);
        auto timing_y_pos = editor_ref->ebuffer.judgeline_position -
                            (timings->at(0)->timestamp -
                             editor_ref->cstatus.current_visual_time_stamp) *
                                editor_ref->canvas_ref->working_map
                                    ->project_reference->config.timeline_zoom *
                                (editor_ref->cstatus.canvas_pasued
                                     ? 1.0
                                     : editor_ref->cstatus.speed_zoom);
        // 画框
        QPointF inner_bound_pos(
            editor_ref->ebuffer.edit_area_start_pos_x -
                inner_bound_size.width() -
                1.5 * editor_ref->canvas_ref->skin.timeinfo_font_size,
            timing_y_pos - inner_bound_size.height() / 2.0);
        auto inner_bound =
            QRectF(inner_bound_pos.x(), inner_bound_pos.y(),
                   inner_bound_size.width(), inner_bound_size.height());

        QColor background_color;

        // 优先判断悬浮选中状态
        if (inner_bound.contains(editor_ref->canvas_ref->mouse_pos)) {
            editor_ref->cstatus.is_hover_timing = true;
            editor_ref->ebuffer.hover_timings = timings;
            background_color = QColor(45, 45, 45, 255);
        } else {
            auto in_select_bound =
                editor_ref->csettings.strict_select
                    ? editor_ref->ebuffer.select_bound.contains(inner_bound)
                    : editor_ref->ebuffer.select_bound.intersects(inner_bound);
            auto it = editor_ref->ebuffer.selected_timingss.find(timings);
            if (in_select_bound ||
                it != editor_ref->ebuffer.selected_timingss.end()) {
                if (it == editor_ref->ebuffer.selected_timingss.end()) {
                    // 未选中则选中此物件
                    editor_ref->ebuffer.selected_timingss.emplace(timings);
                    // 发送更新选中物件信号
                    emit editor_ref->canvas_ref->select_timing(timings);
                }
                // 选中时的颜色
                background_color = QColor(128, 128, 128, 255);
            } else {
                background_color = QColor(0, 0, 0, 255);
            }
        }
        auto &info_params = info_params_list.emplace_back();
        info_params.func_type = FunctionType::MROUNDRECT;
        info_params.xpos = inner_bound.x();
        info_params.ypos = inner_bound.y();
        info_params.width = inner_bound.width();
        info_params.height = inner_bound.height();
        info_params.r = background_color.red();
        info_params.g = background_color.green();
        info_params.b = background_color.blue();
        info_params.a = background_color.alpha();
        info_params.radius = 1.2;
        info_params.is_volatile = true;

        // 画文本
        auto prebpmstrpos_x =
            inner_bound_pos.x() +
            (inner_bound_size.width() - prebpmstr_width) / 2.0;
        double prebpmstrpos_y;
        if (show_speed) {
            prebpmstrpos_y = inner_bound_pos.y() +
                             (inner_bound_size.height() - prebpmstr_height -
                              speedstr_height) /
                                 3.0 +
                             prebpmstr_height;
        } else {
            prebpmstrpos_y =
                inner_bound_pos.y() +
                (inner_bound_size.height() - prebpmstr_height) / 2.0 +
                prebpmstr_height;
        }
        prebpmstrpos_y -= (prebpmstr_height -
                           editor_ref->canvas_ref->skin.timeinfo_font_size) /
                          2.0;

        auto &info_params2 = info_params_list.emplace_back();
        info_params2.func_type = FunctionType::MTEXT;
        info_params2.xpos = prebpmstrpos_x;
        info_params2.ypos = prebpmstrpos_y;
        info_params2.str = u32absbpmstr;
        info_params2.font_family =
            editor_ref->canvas_ref->skin.font_family.c_str();
        info_params2.line_width =
            editor_ref->canvas_ref->skin.timeinfo_font_size;
        info_params2.r = editor_ref->canvas_ref->skin.timeinfo_font_color.red();
        info_params2.g =
            editor_ref->canvas_ref->skin.timeinfo_font_color.green();
        info_params2.b =
            editor_ref->canvas_ref->skin.timeinfo_font_color.blue();
        info_params2.a =
            editor_ref->canvas_ref->skin.timeinfo_font_color.alpha();
        info_params2.is_volatile = true;

        if (show_speed) {
            auto speedstrpos_x =
                inner_bound_pos.x() +
                (inner_bound_size.width() - speedstr_width) / 2.0;
            auto speedstrpos_y =
                prebpmstrpos_y +
                (inner_bound_size.height() - prebpmstr_height -
                 speedstr_height) /
                    3.0 +
                speedstr_height -
                (speedstr_height -
                 editor_ref->canvas_ref->skin.timeinfo_font_size) /
                    2.0;

            auto &info_params = info_params_list.emplace_back();
            info_params.func_type = FunctionType::MTEXT;
            info_params.xpos = speedstrpos_x;
            info_params.ypos = speedstrpos_y;
            info_params.str = u32speedstr;
            info_params.font_family =
                editor_ref->canvas_ref->skin.font_family.c_str();
            info_params.line_width =
                editor_ref->canvas_ref->skin.timeinfo_font_size;
            info_params.r =
                editor_ref->canvas_ref->skin.timeinfo_font_color.red();
            info_params.g =
                editor_ref->canvas_ref->skin.timeinfo_font_color.green();
            info_params.b =
                editor_ref->canvas_ref->skin.timeinfo_font_color.blue();
            info_params.a =
                editor_ref->canvas_ref->skin.timeinfo_font_color.alpha();
            info_params.is_volatile = true;
        }
    }

    // 更新hover信息
    if (!editor_ref->cstatus.is_hover_timing) {
        // 未悬浮在任何一个timing上
        editor_ref->ebuffer.hover_timings = nullptr;
    } else {
        editor_ref->cstatus.is_hover_timing = false;
    }
}
