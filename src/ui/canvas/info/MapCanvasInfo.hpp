#ifndef MMM_MAPCANVASINFO_HPP
#define MMM_MAPCANVASINFO_HPP

#include <glm/glm.hpp>
#include <info/SharedCanvasInfo.hpp>
#include <mmm/map/MMap.hpp>
#include <mmm/project/MProjectConfig.hpp>
#include <tool/BaseTool.hpp>

struct MapInfo {
    // 背景路径
    std::string cover_path{""};

    // 背景暗化
    float darken{.2f};

    // 背景透明度
    float alpha{.8f};
};

struct EditorInfo {
    // 绑定的项目配置
    const MProjectConfig* project_config{nullptr};

    // 当前的编辑模式
    EditMode currentMode;

    // 工具选择
    EditToolType currentEditTool;

    // 当前的map指针
    MMap* map;

    // 轨道布局
    glm::vec4 track_layout;
};

struct MapCanvasInfo : public SharedCanvasInfo {
    // map信息
    MapInfo mapInfo;
    // 编辑信息
    EditorInfo editorInfo;

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
