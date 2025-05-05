#ifndef M_HOLDGENERATOR_H
#define M_HOLDGENERATOR_H

#include <memory>

#include "NoteGenerator.h"

class HoldGenerator : public NoteGenerator {
  // body是否应显示悬浮
  bool should_body_hover(
      const std::shared_ptr<Hold>& obj,
      const std::shared_ptr<HoverInfo>& current_hoverinfo) const;

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
