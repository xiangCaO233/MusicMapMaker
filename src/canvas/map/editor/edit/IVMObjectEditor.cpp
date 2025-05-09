#include "IVMObjectEditor.h"

#include <cmath>
#include <cstdlib>
#include <memory>

#include "../../../../mmm/hitobject/Note/Hold.h"
#include "../../../../mmm/hitobject/Note/HoldEnd.h"
#include "../../../../mmm/hitobject/Note/Note.h"
#include "../../../../mmm/hitobject/Note/rm/ComplexNote.h"
#include "../../MapWorkspaceCanvas.h"
#include "../MapEditor.h"
#include "colorful-log.h"
#include "editor/edit/HitObjectEditor.h"
#include "mmm/hitobject/Note/rm/Slide.h"

// 构造IVMObjectEditor
IVMObjectEditor::IVMObjectEditor(MapEditor* meditor_ref)
    : HitObjectEditor(meditor_ref) {}

// 析构IVMObjectEditor
IVMObjectEditor::~IVMObjectEditor() = default;

// 结束编辑
void IVMObjectEditor::end_edit() {
  // 生成操作并执行到map
  // 生成编辑操作
  ObjEditOperation operation;
  operation.src_objects = editing_src_objects;
  editing_src_objects.clear();

  operation.des_objects = editing_temp_objects;

  // 对map执行操作
  editor_ref->canvas_ref->working_map->execute_edit_operation(operation);

  // 入操作栈
  operation_stack.emplace(operation);
  // 清理正在编辑的物件
  editing_temp_objects.clear();
  current_edit_complex = nullptr;
  current_edit_object = nullptr;
  // 清理快照
  info_shortcuts.clear();
}

// 鼠标按下事件-传递
void IVMObjectEditor::mouse_pressed(QMouseEvent* e) {
  // 抄抄ivm编辑逻辑
  // 鼠标按钮
  auto button = e->button();
  switch (button) {
    case Qt::LeftButton: {
      // 若有悬浮于某物件则设置此物件为编辑中的物件
      if (!current_edit_object && editor_ref->ebuffer.hover_object_info) {
        current_edit_object = editor_ref->ebuffer.hover_object_info->hoverobj;

        if (!editor_ref->ebuffer.selected_hitobjects.empty()) {
          // 备份到src-放到编辑缓存
          for (const auto& selection :
               editor_ref->ebuffer.selected_hitobjects) {
            if (!selection->is_note) continue;
            // 克隆一份用于编辑缓存显示虚影
            auto cloned_obj = std::shared_ptr<HitObject>(selection->clone());
            // 原来的物件放到src源物件表中-即将删除
            editing_src_objects.insert(selection);
            // 记录物件快照
            info_shortcuts.insert(
                std::shared_ptr<HitObject>(cloned_obj->clone()));
            editing_temp_objects.insert(cloned_obj);
          }
          editor_ref->ebuffer.selected_hitobjects.clear();
          return;
        }

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
        // 左键press放置-drag移动位置-release生效
        // 查询当前鼠标位置最近的拍-在最近的分拍线放置
        auto time = nearest_divisor_time();
        if (std::abs(time - (-1.0)) != 1e-16) {
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
              auto time = nearest_divisor_time();
              if (std::abs(time - (-1.0)) == 1e-16) return;

              if (long_note_edit_mode) {
                auto note =
                    std::dynamic_pointer_cast<Note>(current_edit_object);
                if (!note) return;
                // 修改缓存中的面条物件的持续时间
                // 判断缓存物件是面条还是滑键
                if (note->note_type == NoteType::HOLD) {
                  // 1--缓存物件是面条
                  auto hold = std::static_pointer_cast<Hold>(note);
                  if (!hold) return;

                  // 判断当前编辑物件的轨道与当前鼠标位置的轨道
                  if (note->orbit == editor_ref->cstatus.mouse_pos_orbit) {
                    // 1.1当前编辑物件的轨道与当前鼠标位置的轨道相同
                    // 判断当前编辑物件头的时间到鼠标时间>=0
                    if (time - note->timestamp >= 0) {
                      // 1.1.1快照时间到鼠标时间>=0时修改缓存物件面条中的持续时间
                      hold->hold_time = time - note->timestamp;
                      // 修改面尾位置
                      hold->hold_end_reference->timestamp =
                          hold->timestamp + hold->hold_time;

                    } else {
                      // 1.1.2快照时间到鼠标时间<0面条持续时间非法,跳过修改并提示
                      XERROR(QString("面条持续时间非法[%1]ms")
                                 .arg(time - note->timestamp)
                                 .toStdString());
                      return;
                    }
                  } else {
                    // 1.2当前编辑物件的轨道与当前鼠标位置的轨道不同
                    // 判断当前编辑面条的面尾时间与鼠标时间是否相同
                    if (hold->hold_end_reference->timestamp == time) {
                      // 1.2.1当前编辑面条的面尾时间与鼠标时间相同
                      // 判断是否生成组合键
                      if (!current_edit_complex) {
                        // 未生成组合键-
                        if (hold->hold_time == 0) {
                          // 处于未定义面条状态
                          // 直接去掉面条改为滑键
                          // 创建滑键
                          auto slide = std::make_shared<Slide>(
                              time, hold->orbit,
                              editor_ref->cstatus.mouse_pos_orbit -
                                  hold->orbit);
                          // 修改当前正在编辑的物件为此滑键
                          current_edit_object = slide;

                          // 初始化滑尾
                          slide->slide_end_reference =
                              std::make_shared<SlideEnd>(slide);

                          // 清理编辑缓存
                          editing_temp_objects.clear();

                          // 清理快照
                          info_shortcuts.clear();

                          // 记录物件克隆快照
                          info_shortcuts.insert(std::shared_ptr<HitObject>(
                              current_edit_object->clone()));

                          // 放入编辑缓存
                          editing_temp_objects.insert(current_edit_object);

                        } else {
                          // 已定义面条-在面条头位置生成组合键
                          current_edit_complex = std::make_shared<ComplexNote>(
                              hold->timestamp, hold->orbit);
                          // 设置当前面条为组合键头
                          hold->compinfo = ComplexInfo::HEAD;

                          // 在组合键中添加滑键
                          // 创建面条尾轨道到鼠标轨道的滑键
                          auto slide = std::make_shared<Slide>(
                              time, hold->orbit,
                              editor_ref->cstatus.mouse_pos_orbit -
                                  hold->orbit);
                          // 修改当前正在编辑的物件为此滑键
                          current_edit_object = slide;

                          // 初始化滑尾
                          slide->slide_end_reference =
                              std::make_shared<SlideEnd>(slide);

                          // 设置当前滑键为组合键尾
                          slide->compinfo = ComplexInfo::END;

                          // 记录物件快照
                          info_shortcuts.insert(std::shared_ptr<HitObject>(
                              current_edit_object->clone()));

                          // 放入编辑缓存
                          editing_temp_objects.insert(current_edit_object);

                          update_current_comp();
                        }
                      } else {
                        // 已存在组合键且组合键尾为面条
                        // 在面尾时间处添加一个滑键-更新当前编辑的物件为此新物件
                        // 在组合键中添加滑键
                        // 创建面条尾轨道到鼠标轨道的滑键
                        // 上一个面条处于组合键中-必然不是位于组合头
                        hold->compinfo = ComplexInfo::BODY;

                        auto slide = std::make_shared<Slide>(
                            time, hold->orbit,
                            editor_ref->cstatus.mouse_pos_orbit - hold->orbit);
                        // 修改当前正在编辑的物件为此滑键
                        current_edit_object = slide;

                        // 初始化滑尾
                        slide->slide_end_reference =
                            std::make_shared<SlideEnd>(slide);

                        // 设置当前滑键为组合键尾
                        slide->compinfo = ComplexInfo::END;

                        // 记录物件快照
                        info_shortcuts.insert(std::shared_ptr<HitObject>(
                            current_edit_object->clone()));

                        // 放入编辑缓存
                        editing_temp_objects.insert(current_edit_object);
                        update_current_comp();
                      }
                    } else {
                      // 1.2.2当前编辑面条的面尾时间与鼠标时间不同-组合物件非法,跳过修改并提示
                      XERROR("往哪滑呢😠 " +
                             QString("从面条尾-time[%1],orbit[%2]添加滑键到-"
                                     "time[%3],orbit[%4],slideorbit[%5]非法")
                                 .arg(hold->hold_end_reference->timestamp)
                                 .arg(hold->orbit)
                                 .arg(time)
                                 .arg(editor_ref->cstatus.mouse_pos_orbit)
                                 .arg(editor_ref->cstatus.mouse_pos_orbit -
                                      hold->orbit)
                                 .toStdString());
                    }
                  }
                } else {
                  // 2--缓存物件是滑键
                  // 判断当前编辑滑键的时间与鼠标时间是否相同
                  auto slide = std::static_pointer_cast<Slide>(note);
                  if (!slide) return;
                  if (slide->timestamp == time) {
                    // 2.1当前编辑滑键的时间与鼠标时间相同
                    // 修改当前滑键的滑动参数
                    auto silde_param =
                        editor_ref->cstatus.mouse_pos_orbit - slide->orbit;
                    if (silde_param == 0) {
                      XWARN("不允许原地滑😠");
                      return;
                    } else {
                      // 更新滑键滑动轨道数
                      slide->slide_parameter = silde_param;
                      slide->slide_end_reference->endorbit =
                          slide->orbit + slide->slide_parameter;
                    }
                  } else {
                    // 2.2当前编辑滑键的时间与鼠标时间不同
                    // 判断滑键尾轨道与鼠标位置轨道是否相同
                    if (slide->slide_end_reference->endorbit ==
                        editor_ref->cstatus.mouse_pos_orbit) {
                      // 2.2.1滑键尾轨道与鼠标位置轨道相同

                      // 判断是否生成组合键
                      if (!current_edit_complex) {
                        // 未生成组合键
                        current_edit_complex = std::make_shared<ComplexNote>(
                            slide->timestamp, slide->orbit);
                        // 当前滑键为组合键头
                        slide->compinfo = ComplexInfo::HEAD;
                        // 生成组合键并新增长条到组合键
                      } else {
                        // 已有组合键
                        // 修改当前滑键为组合键body
                        slide->compinfo = ComplexInfo::BODY;
                      }
                      // 新增长条到组合键
                      auto hold = std::make_shared<Hold>(
                          slide->timestamp, editor_ref->cstatus.mouse_pos_orbit,
                          time - slide->timestamp);
                      // 面尾--
                      hold->hold_end_reference =
                          std::make_shared<HoldEnd>(hold);
                      // 设置新增面条为组合键尾
                      hold->compinfo = ComplexInfo::END;
                      // 更新当前正在编辑的物件
                      current_edit_object = hold;

                      // 记录物件快照
                      info_shortcuts.insert(std::shared_ptr<HitObject>(
                          current_edit_object->clone()));
                      // 放入编辑缓存
                      editing_temp_objects.insert(current_edit_object);

                    } else {
                      // 2.2.2滑键尾轨道与鼠标位置轨道不同
                      // 非法绘制
                      XERROR("往哪加条呢😠 " +
                             QString("从滑尾位置-time[%1],orbit[%2]添加长条到-"
                                     "time[%3],orbit[%4],holdtime[%5]非法")
                                 .arg(slide->timestamp)
                                 .arg(slide->slide_end_reference->endorbit)
                                 .arg(time)
                                 .arg(editor_ref->cstatus.mouse_pos_orbit)
                                 .arg(time - slide->timestamp)
                                 .toStdString());
                      return;
                    }
                  }
                }
              } else {
                // 生成虚影0长面条
                auto hold = std::make_shared<Hold>(
                    time, editor_ref->cstatus.mouse_pos_orbit, 0);
                // 面尾--
                hold->hold_end_reference = std::make_shared<HoldEnd>(hold);

                current_edit_object = hold;

                // 初始化正在编辑的组合键
                // current_edit_complex = std::make_shared<ComplexNote>(
                //     time, editor_ref->cstatus.mouse_pos_orbit);

                // 生成一个note信息快照记录鼠标按下时的位置信息
                // 记录物件快照
                info_shortcuts.insert(
                    std::shared_ptr<HitObject>(current_edit_object->clone()));
                editing_src_objects.clear();

                // 放入编辑缓存
                editing_temp_objects.insert(current_edit_object);

                // 添加当前物件到组合键中
                // current_edit_complex->child_notes.emplace_back(
                //     std::static_pointer_cast<Hold>(current_edit_object));
                XINFO("开始编辑面条");
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

// 更新正编辑的当前组合键
void IVMObjectEditor::update_current_comp() {
  // 将当前编辑缓存的全部物件加入组合键子键中
  current_edit_complex->child_notes.clear();
  for (const auto& obj : editing_temp_objects) {
    auto temp_note = std::dynamic_pointer_cast<Note>(obj);
    if (!temp_note) continue;
    current_edit_complex->child_notes.emplace_back(temp_note);
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
        end_edit();

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
        end_edit();
        long_note_edit_mode = false;
        XINFO("结束编辑面条");
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
        switch (tempnote->note_type) {
          case NoteType::HOLD: {
            auto hold = std::static_pointer_cast<Hold>(tempnote);
            hold->hold_end_reference->timestamp =
                tempnote->timestamp + hold->hold_time;
            break;
          }
          case NoteType::SLIDE: {
            auto slide = std::static_pointer_cast<Slide>(tempnote);
            slide->slide_end_reference->timestamp = slide->timestamp;
            slide->slide_end_reference->endorbit =
                slide->orbit + slide->slide_parameter;
            break;
          }
        }
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
