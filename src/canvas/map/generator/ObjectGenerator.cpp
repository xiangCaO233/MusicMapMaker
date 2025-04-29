#include "ObjectGenerator.h"

#include "../MapWorkspaceCanvas.h"
#include "../editor/MapEditor.h"
#include "mmm/hitobject/Note/Note.h"

// 图形渲染数据队列(确定层级)
std::queue<ObjectRenderData> ObjectGenerator::shape_queue;

// 共用缓存节点映射表
std::map<std::shared_ptr<HitObject>, QRectF> ObjectGenerator::temp_node_map;

// 构造ObjectGenerator
ObjectGenerator::ObjectGenerator(std::shared_ptr<MapEditor>& editor)
    : editor_ref(editor) {}

// 析构ObjectGenerator
ObjectGenerator::~ObjectGenerator() = default;

// 转移全部缓存节点到队列中
void ObjectGenerator::dump_nodes_to_queue() {
  for (const auto& [notereference, noderect] : temp_node_map) {
    auto is_node_hover = noderect.contains(editor_ref->canvas_ref->mouse_pos);
    auto is_node_in_selected_bound =
        editor_ref->strict_select
            ? editor_ref->select_bound.contains(noderect)
            : editor_ref->select_bound.intersects(noderect);
    auto noteref_selectit = editor_ref->selected_hitobjects.find(notereference);
    if (is_node_hover) {
      // 使用hover纹理
      // 将node直接加入当前队列
      shape_queue.emplace(noderect.x(), noderect.y(), noderect.width(),
                          noderect.height(),
                          editor_ref->canvas_ref->skin.get_object_texture(
                              TexType::NODE, ObjectStatus::HOVER));

      // 选中组合键的节点(节点是相当于那一物件的尾)
      editor_ref->hover_hitobject_info =
          std::make_shared<std::pair<std::shared_ptr<HitObject>, bool>>(
              notereference, true);
      editor_ref->is_hover_note = true;
    } else if (is_node_in_selected_bound ||
               noteref_selectit != editor_ref->selected_hitobjects.end()) {
      // 选中此节点对应的物件
      editor_ref->selected_hitobjects.emplace(notereference);
      // 使用选中纹理
      // 将node直接加入当前队列
      shape_queue.emplace(noderect.x(), noderect.y(), noderect.width(),
                          noderect.height(),
                          editor_ref->canvas_ref->skin.get_object_texture(
                              TexType::NODE, ObjectStatus::SELECTED));
    } else {
      // 使用常规纹理
      // 将node直接加入当前队列
      shape_queue.emplace(noderect.x(), noderect.y(), noderect.width(),
                          noderect.height(),
                          editor_ref->canvas_ref->skin.get_object_texture(
                              TexType::NODE, ObjectStatus::COMMON));
    }
  }
  // 并清空节点缓存
  temp_node_map.clear();
}

// 生成物件渲染指令
void ObjectGenerator::generate(const std::shared_ptr<HitObject>& hitobject) {
  hitobject->accept_generate(*this);
}

void ObjectGenerator::generate(Note& note) {}
void ObjectGenerator::generate(Hold& hold) {}
void ObjectGenerator::generate(Slide& slide) {}
