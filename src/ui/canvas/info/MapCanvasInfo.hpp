#ifndef MMM_MAPCANVASINFO_HPP
#define MMM_MAPCANVASINFO_HPP

#include <info/SharedCanvasInfo.hpp>
#include <mmm/project/MProjectConfig.hpp>
#include <tool/BaseTool.hpp>

struct EditorInfo {
    EditMode currentMode;
    // 工具选择/默认时间线
    EditToolType currentEditTool;
};

struct MapCanvasInfo : public SharedCanvasInfo {
    // 编辑信息
    EditorInfo editorInfo;
};

#endif  // MMM_MAPCANVASINFO_HPP
