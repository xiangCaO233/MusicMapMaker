#ifndef M_EDITOR_ENUMERATIONS_H
#define M_EDITOR_ENUMERATIONS_H

#include <cstdint>

enum class MouseEditMode : int32_t {
    // 右键删除
    // 点击放置,拖动时调整时间戳
    PLACE_NOTE = 1,
    // 拖动放置(组合键,面条,长条,滑键)
    PLACE_LONGNOTE = 2,

    // 放置timing-左键直接放置-绝对bpm
    // 右键放置-变速timing
    // 当即选中,可编辑
    PLACE_TIMING = 3,

    // 选择-可框选选中物件等
    SELECT = 4,
    // 仅预览
    NONE = 5,
};

// 鼠标正在操作的区域
enum class MouseOperationArea {
    // 编辑区
    EDIT,
    // 预览区
    PREVIEW,
    // 信息区
    INFO,
};

#endif  // M_EDITOR_ENUMERATIONS_H
