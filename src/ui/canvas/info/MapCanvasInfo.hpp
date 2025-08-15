#ifndef MMM_MAPCANVASINFO_HPP
#define MMM_MAPCANVASINFO_HPP

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
    // 当前的编辑模式
    EditMode currentMode;

    // 工具选择
    EditToolType currentEditTool;

    // 当前的map指针
    MMap *map;
};

struct MapCanvasInfo : public SharedCanvasInfo {
    // map信息
    MapInfo mapInfo;
    // 编辑信息
    EditorInfo editorInfo;
};

#endif  // MMM_MAPCANVASINFO_HPP
