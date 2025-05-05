#ifndef M_NOTEGENERATOR_H
#define M_NOTEGENERATOR_H

#include <memory>

#include "../ObjectGenerator.h"

struct HoverInfo;

class NoteGenerator : public ObjectGenerator {
 public:
  // 构造NoteGenerator
  explicit NoteGenerator(std::shared_ptr<MapEditor>& editor);

  // 析构NoteGenerator
  ~NoteGenerator() override;

  // 节点尺寸
  QSizeF node_size;

  // 物件头的位置尺寸
  QRectF head_rect;

  // 物件头的纹理
  std::shared_ptr<TextureInstace> head_texture;

  // 物件引用
  std::shared_ptr<HitObject> objref;

  // 生成物件
  void generate(Note& note) override;

  // 完成生成-添加到渲染列表
  void object_enqueue() override;
};

#endif  // M_NOTEGENERATOR_H
