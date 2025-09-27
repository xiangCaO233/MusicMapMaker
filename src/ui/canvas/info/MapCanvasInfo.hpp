#ifndef MMM_MAPCANVASINFO_HPP
#define MMM_MAPCANVASINFO_HPP

#include <glm/glm.hpp>
#include <info/SharedCanvasInfo.hpp>
#include <mmm/project/MProjectConfig.hpp>
#include <tool/BaseTool.hpp>

class MMap;
class MMapEditor;
class AudioLoadCallback;
struct MapInfo {
    // 背景路径
    std::string cover_path{""};

    // 背景暗化
    float darken{.2f};

    // 背景透明度
    float alpha{1.f};
};

// 滚动行为信息
struct ScrollInfo {
    // 播放滚动速度
    float scroll_speed{1.f};

    // 时间线缩放
    float timeline_zoom{.8f};

    // 滑轮自然滚动
    bool scroll_natural{false};

    // 滑轮页滚动步长倍率
    float pageScrollStepRatio{1.f};

    // 滑轮时间线缩放滚动步长
    float staticTimelineScrollRatio{8.333e-5f};
    float timelineScrollRatio{1.f};

    bool operator==(const ScrollInfo& other) const = default;
};

// 预览区信息
struct PreviewAreaInfo {
    // 区域相对主区域的范围倍率
    // 2.0f ~ 7.5f
    float areaRatio{3.f};
    // 主区域在预览区的位置
    // 0.0f ~ 1.0f
    float mainAreaPos{.5f};

    bool operator==(const PreviewAreaInfo& other) const = default;
};

class MSkin;
struct EditorInfo {
    // 绑定的项目配置
    const MProjectConfig* project_config{nullptr};

    // 工具选择
    EditToolType currentEditTool;

    // 判定线位置
    float judgeline_pos{.2f};

    // 滚动是否吸附到分拍线
    bool magnet_to_divisor{false};

    // 滚动行为信息
    ScrollInfo scrollInfo;

    // 预览区信息
    PreviewAreaInfo previewAreaInfo;

    // 当前的map指针
    MMap* map;

    // 轨道布局
    glm::vec4 track_layout;

    // 编辑器皮肤
    MSkin* skin;
};

struct MapCanvasInfo : public SharedCanvasInfo {
    ~MapCanvasInfo() override = default;

    // map信息
    MapInfo mapInfo;
    // 编辑信息
    EditorInfo editorInfo;
    // 音频回调指针
    AudioLoadCallback* audio_callback;

    // 绑定项目配置
    void bindProjectConfig(const MProjectConfig* cfg) {
        editorInfo.project_config = cfg;
        // 更新一次轨道布局
        update_trackLayout();
    }

    void update_trackLayout() {
        if (editorInfo.project_config) {
            auto x = baseInfo.canvasSize.width() *
                     editorInfo.project_config->canvas_config.canvas_layout
                         .w;  // left
            auto y = baseInfo.canvasSize.height() *
                     editorInfo.project_config->canvas_config.canvas_layout
                         .x;  // top
            auto w = baseInfo.canvasSize.width() *
                     (editorInfo.project_config->canvas_config.canvas_layout.y -
                      editorInfo.project_config->canvas_config.canvas_layout
                          .w);  // right - left
            auto h = baseInfo.canvasSize.height() *
                     (editorInfo.project_config->canvas_config.canvas_layout.z -
                      editorInfo.project_config->canvas_config.canvas_layout
                          .x);  // bottom - top
            editorInfo.track_layout = {x, y, w, h};
        }
    }
};

#endif  // MMM_MAPCANVASINFO_HPP
