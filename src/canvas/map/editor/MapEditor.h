#ifndef M_MAPEDITOR_H
#define M_MAPEDITOR_H

#include <qflags.h>
#include <qnamespace.h>
#include <qpoint.h>

#include <QPointF>
#include <QRectF>
#include <memory>
#include <stack>

#include "CanvasSettings.h"
#include "CanvasStatus.h"
#include "EditorBuffer.h"
#include "editor/HitObjectEditor.h"
#include "editor/TimingEditor.h"

class MapWorkspaceCanvas;
class TextureInstace;

enum class EditMethodPreference;

enum EditOperationType {
  HITOBJECT,
  TIMING,
};

// 编辑器
class MapEditor {
 public:
  // 构造MapEditor
  explicit MapEditor(MapWorkspaceCanvas* canvas);
  // 析构MapEditor
  virtual ~MapEditor();

  // 画布引用
  MapWorkspaceCanvas* canvas_ref;

  // 信息
  // 画布状态
  CanvasStatus cstatus;
  // 画布设置
  CanvasSettings csettings;
  // 编辑器缓存
  EditorBuffer ebuffer;

  // 鼠标当前的编辑模式
  MouseEditMode edit_mode;

  // 编辑方式-ivm或mmm
  EditMethodPreference edit_method;

  // 编辑
  // 物件编辑器
  HitObjectEditor obj_editor;
  // timing编辑器
  TimingEditor timing_editor;
  // 操作类型栈
  std::stack<EditOperationType> operation_type_stack;
  // 撤回类型栈
  std::stack<EditOperationType> undo_type_stack;

  // 撤销
  void undo();
  // 重做
  void redo();

  // 其他函数
  // 画布更新尺寸
  void update_size(const QSize& current_canvas_size);
  // 更新区域信息
  void update_areas();
  // 鼠标按下
  void mouse_pressed(QMouseEvent* e);
  // 更新时间线缩放-滚动
  void scroll_update_timelinezoom(int scrolldy);
  // 吸附到附近分拍线
  void scroll_magnet_to_divisor(int scrolldy);
  // 更新谱面位置
  void update_timepos(int scrolldy, bool is_shift_down);
  // 更新选中信息
  void update_selections(bool is_ctrl_down);
  // 更新选中区域
  void update_selection_area(QPoint&& p, bool ctrl_down);
};

#endif  // M_MAPEDITOR_H
