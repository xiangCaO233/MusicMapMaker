#ifndef M_EDITORBUFFER_H
#define M_EDITORBUFFER_H

#include <memory>
#include <set>
#include <unordered_set>
#include <vector>

#include "../../../../mmm/Beat.h"
#include "../../../../mmm/hitobject/HitObject.h"
#include "../../../../mmm/timing/Timing.h"
#include "../../../texture/Texture.h"
#include "HoverInfo.h"

struct EditorBuffer {
    // 上一次编辑区绘制的时间区域
    double current_time_area_start{0.0};
    double current_time_area_end{0.0};

    // 当前正在使用的绝对timing--非变速timing
    std::shared_ptr<Timing> current_abs_timing;

    // 附近的两个变速timing
    std::shared_ptr<Timing> pre_speed_timing;
    std::shared_ptr<Timing> next_speed_timing;

    // 当前页面的拍
    std::vector<std::shared_ptr<Beat>> current_beats;

    // 编辑区x起始位置
    double edit_area_start_pos_x;

    // 编辑区宽度
    double edit_area_width;

    // 预览区x起始位置
    double preview_area_start_pos_x;

    // 预览区宽度
    double preview_area_width;

    // 判定线位置
    double judgeline_position;

    // 判定线视觉位置
    double judgeline_visual_position;

    // 轨道数
    int32_t max_orbit;

    // 物件缓存
    std::multiset<std::shared_ptr<HitObject>, HitObjectComparator>
        buffer_objects;

    // 预览物件缓存
    std::multiset<std::shared_ptr<HitObject>, HitObjectComparator>
        buffer_preview_objects;

    // 鼠标悬停位置的物件信息
    std::shared_ptr<HoverObjectInfo> hover_object_info{nullptr};

    // 鼠标悬停位置的timings信息(可能是两个同时间timing)
    std::vector<std::shared_ptr<Timing>>* hover_timings{nullptr};

    // 哈希函数和比较函数
    struct VectorSharedPtrTimingHash {
        std::size_t operator()(
            const std::vector<std::shared_ptr<Timing>>* ptr) const {
            if (ptr == nullptr) {
                return 0;  // 统一处理 nullptr
            }

            std::size_t seed = ptr->size();
            for (const auto& shared_timing : *ptr) {
                // 使用 combine 技术混合哈希值
                if (shared_timing) {
                    const Timing& timing = *shared_timing;
                    seed ^= std::hash<double>{}(timing.timestamp) + 0x9e3779b9 +
                            (seed << 6) + (seed >> 2);
                    seed ^= std::hash<double>{}(timing.bpm) + 0x9e3779b9 +
                            (seed << 6) + (seed >> 2);
                    seed ^= std::hash<double>{}(timing.basebpm) + 0x9e3779b9 +
                            (seed << 6) + (seed >> 2);
                } else {
                    // 统一处理 nullptr
                    seed ^= 0x9e3779b9 + (seed << 6) + (seed >> 2);
                }
            }
            return seed;
        }
    };
    struct VectorSharedPtrTimingEqual {
        bool operator()(const std::vector<std::shared_ptr<Timing>>* lhs,
                        const std::vector<std::shared_ptr<Timing>>* rhs) const {
            // 处理 nullptr 情况
            if (lhs == rhs) return true;
            if (lhs == nullptr || rhs == nullptr) return false;

            // 比较大小
            if (lhs->size() != rhs->size()) return false;

            // 逐个比较元素
            for (size_t i = 0; i < lhs->size(); ++i) {
                const auto& l = (*lhs)[i];
                const auto& r = (*rhs)[i];

                // 处理 nullptr 情况
                if (!l && !r) continue;
                if (!l || !r) return false;

                // 比较 Timing 对象的属性
                if (l->timestamp != r->timestamp || l->bpm != r->bpm ||
                    l->basebpm != r->basebpm) {
                    return false;
                }
            }

            return true;
        }
    };

    // 选中的timings
    std::unordered_set<std::vector<std::shared_ptr<Timing>>*,
                       VectorSharedPtrTimingHash, VectorSharedPtrTimingEqual>
        selected_timingss{nullptr};

    // 选中的物件
    // 哈希函数和比较函数
    struct RawPtrHash {
        size_t operator()(const std::shared_ptr<HitObject>& ptr) const {
            return std::hash<HitObject*>()(ptr.get());
        }
    };

    struct RawPtrEqual {
        bool operator()(const std::shared_ptr<HitObject>& lhs,
                        const std::shared_ptr<HitObject>& rhs) const {
            return lhs.get() == rhs.get();
        }
    };
    std::unordered_set<std::shared_ptr<HitObject>, RawPtrHash, RawPtrEqual>
        selected_hitobjects;

    // 物件头的纹理
    std::shared_ptr<TextureInstace> head_texture;

    // 轨道宽度
    double orbit_width;
    double preview_orbit_width;

    // 依据轨道宽度自动适应物件纹理尺寸
    // 物件尺寸缩放--相对于纹理尺寸
    double width_scale;
    double preview_width_scale;

    // 不大于1--不放大纹理
    double object_size_scale;
    double preview_object_size_scale;

    // 选中框定位点
    std::shared_ptr<std::pair<QPointF, QPointF>> select_bound_locate_points;

    // 选中区域
    QRectF select_bound;
};

#endif  // M_EDITORBUFFER_H
