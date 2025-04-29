#ifndef M_HOLDGENERATOR_H
#define M_HOLDGENERATOR_H

#include <memory>

#include "NoteGenerator.h"

class HoldGenerator : public NoteGenerator {
 public:
  // 构造HoldGenerator
  explicit HoldGenerator(std::shared_ptr<MapEditor>& editor);
  // 析构HoldGenerator
  ~HoldGenerator() override;

  // 面身的位置尺寸
  QRectF hold_vert_body_rect;

  // 面身的实际使用纹理
  std::shared_ptr<TextureInstace> hold_vert_body_texture;

  // 面尾的位置尺寸
  QRectF hold_end_rect;

  // 面尾的实际使用纹理
  std::shared_ptr<TextureInstace> hold_end_texture;

  // 生成面条
  void generate(Hold& hold) override;
};

#endif  // M_HOLDGENERATOR_H
