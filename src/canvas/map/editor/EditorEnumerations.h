#ifndef M_EDITOR_ENUMERATIONS_H
#define M_EDITOR_ENUMERATIONS_H

#include <cstdint>

enum class MouseEditMode : int32_t {
  // 点击放置,拖动时调整时间戳
  PLACE_NOTE = 1,
  // 拖动放置(组合键,面条,长条,滑键)
  PLACE_LONGNOTE = 2,
  // 选择-可框选选中物件等
  SELECT = 3,
  // 仅预览
  NONE = 4,
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
