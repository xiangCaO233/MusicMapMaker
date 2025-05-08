#include "IVMObjectEditor.h"

#include <cmath>
#include <cstdlib>
#include <memory>
#include <utility>

#include "../../MapWorkspaceCanvas.h"
#include "../MapEditor.h"
#include "colorful-log.h"
#include "editor/edit/HitObjectEditor.h"
#include "mmm/hitobject/Note/Note.h"

// 构造IVMObjectEditor
IVMObjectEditor::IVMObjectEditor(MapEditor* meditor_ref)
    : HitObjectEditor(meditor_ref) {}

// 析构IVMObjectEditor
IVMObjectEditor::~IVMObjectEditor() = default;

// 鼠标按下事件-传递
void IVMObjectEditor::mouse_pressed(QMouseEvent* e) {
  // 抄抄ivm编辑逻辑
  // 鼠标按钮
  auto button = e->button();
  switch (button) {
    case Qt::LeftButton: {
      // 左键press放置-drag移动位置-release生效
      // TODO(xiang 2025-05-08): 若有悬浮于某物件则设置此物件为编辑中的物件
      if (!current_edit_object && editor_ref->ebuffer.hover_object_info) {
        current_edit_object = editor_ref->ebuffer.hover_object_info->hoverobj;

        // 清空当前编辑的源物件表
        editing_src_objects.clear();

        // 克隆一份用于编辑缓存显示虚影
        auto cloned_obj =
            std::shared_ptr<HitObject>(current_edit_object->clone());

        // 原来的物件放到src源物件表中-即将删除
        editing_src_objects.insert(current_edit_object);

        // 克隆份放入编辑缓存
        editing_temp_objects.insert(cloned_obj);
        // 记录物件快照
        info_shortcuts.insert(std::shared_ptr<HitObject>(cloned_obj->clone()));

        XINFO(QString("编辑已有物件->[%1]")
                  .arg(current_edit_object->timestamp)
                  .toStdString());
      } else {
        // 查询当前鼠标位置最近的拍-在最近的分拍线放置
        auto time = nearest_divisor_time();
        if (!(std::abs(time - (-1.0)) < 1e-16)) {
          // 根据编辑模式向此时间戳添加物件
          switch (editor_ref->edit_mode) {
            case MouseEditMode::PLACE_NOTE: {
              // 否则添加新物件
              // 直接放置物件
              // 在对应轨道生成物件
              current_edit_object = std::make_shared<Note>(
                  time, editor_ref->cstatus.mouse_pos_orbit);

              // 生成一个note信息快照记录鼠标按下时的位置信息
              // 记录物件快照
              info_shortcuts.insert(
                  std::shared_ptr<HitObject>(current_edit_object->clone()));

              // 放入编辑缓存
              editing_src_objects.clear();
              editing_temp_objects.insert(
                  std::shared_ptr<HitObject>(current_edit_object->clone()));

              XINFO(QString("新增物件->[%1]")
                        .arg(current_edit_object->timestamp)
                        .toStdString());
              break;
            }
            case MouseEditMode::PLACE_LONGNOTE: {
              // 放0长面条头-进入面条编辑状态-此时不可切换模式(直到右键面条编辑结束)
              if (long_note_edit_mode) {
                // 修改缓存中的面条物件的持续时间
              } else {
                long_note_edit_mode = true;
              }
              break;
            }
          }
        }
      }

      // if (behavior == QEvent::MouseButtonRelease) {
      // }

      break;
    }
    case Qt::RightButton: {
      // 右键删除
      // 编辑面条模式右键结束
      break;
    }
  }
}
void IVMObjectEditor::mouse_released(QMouseEvent* e) {
  // 鼠标按钮
  auto button = e->button();
  switch (button) {
    case Qt::LeftButton: {
      // 释放的是左键
      //
      //
      // 若为面条编辑模式-
      // 应用更改面条的结果到编辑缓存
      //
      // 若为单键编辑模式-
      // 直接应用编辑缓存的修改到map
      if (!long_note_edit_mode) {
        // 生成操作并执行到map
        // 生成编辑操作
        ObjEditOperation operation;
        operation.src_objects = editing_src_objects;
        editing_src_objects.clear();
        operation.des_objects = editing_temp_objects;
        editing_temp_objects.clear();

        // 对map执行操作
        editor_ref->canvas_ref->working_map->execute_edit_operation(operation);

        // 入操作栈
        operation_stack.emplace(operation);
        // 清理正在编辑的物件
        current_edit_object = nullptr;
        // 清理快照
        info_shortcuts.clear();

        XINFO(QString("编辑结束生成物件操作").toStdString());
      } else {
        // 编辑面条模式并非结束
      }
      break;
    }
    case Qt::RightButton: {
      // 释放的是右键
      // 若为面条编辑模式则面条编辑结束
      // 否则删除鼠标悬浮位置的物件
      if (!long_note_edit_mode) {
      } else {
        // 结束面条编辑
        long_note_edit_mode = false;
      }
    }
  }
}

// 鼠标拖动事件-传递
void IVMObjectEditor::mouse_dragged(QMouseEvent* e) {
  auto time = nearest_divisor_time();
  if (!(std::abs(time - (-1.0)) < 1e-16)) {
    // 根据编辑模式修改所有编辑缓存物件的
    // note
    // 当前编辑物件的相对位置和时间变化值
    // hold
    // 或持续时间
    // slide
    // 或滑动轨道数
    // 计算相对变化
    auto note = std::dynamic_pointer_cast<Note>(current_edit_object);
    if (note) {
      switch (note->note_type) {
        case NoteType::NOTE: {
          // 拖动单键
          // 相对移动的时间和轨道
          auto rtime = time - note->timestamp;
          auto rorbit = editor_ref->cstatus.mouse_pos_orbit - note->orbit;

          // 将编辑缓存中的所有可转化为Note的物件应用此变化
          // 同时遍历缓存和快照-
          auto it = editing_temp_objects.begin();
          auto info_it = info_shortcuts.begin();
          while (it != editing_temp_objects.end() &&
                 info_it != info_shortcuts.end()) {
            auto tempnote = std::dynamic_pointer_cast<Note>(*it);
            auto info = std::dynamic_pointer_cast<Note>(*info_it);
            if (tempnote) {
              auto srctime = tempnote->timestamp;
              auto srco = tempnote->orbit;
              tempnote->timestamp = info->timestamp + rtime;
              tempnote->orbit = info->orbit + rorbit;
              // TODO(xiang 2025-05-08):
              // 面条和滑键需要同时更新面尾和滑尾-组合键的时间戳也需要更新
              XINFO(QString("移动物件t[%1]->t[%2]  o[%3]->o[%4]")
                        .arg(srctime)
                        .arg(tempnote->timestamp)
                        .arg(srco)
                        .arg(tempnote->orbit)
                        .toStdString());
            }
            ++it;
            ++info_it;
          }
        }
        case NoteType::HOLD: {
          // 拖动长条-只看鼠标时间-不管轨道
          // 修改所有编辑缓存中的面条的相对持续时间
        }
      }
    }
    // 已在map编辑器过滤
    // switch (editor_ref->edit_mode) {
    //     // 编辑物件的两个模式-拖动事件
    //   case MouseEditMode::PLACE_LONGNOTE:
    //   case MouseEditMode::PLACE_NOTE: {
    //     break;
    //   }
    // }
  }
}
