#ifndef M_SLIDEGENERATOR_H
#define M_SLIDEGENERATOR_H

#include <memory>

#include "NoteGenerator.h"

class SlideGenerator : public NoteGenerator {
 public:
  // 构造SlideGenerator
  explicit SlideGenerator(std::shared_ptr<MapEditor>& editor);
  // 析构SlideGenerator
  ~SlideGenerator() override;

  // 滑身的位置尺寸
  QRectF slide_hori_body_rect;

  // 滑身的纹理
  std::shared_ptr<TextureInstace> slide_hori_body_texture;

  // 滑尾的位置尺寸
  QRectF slide_end_rect;

  // 滑尾的纹理
  std::shared_ptr<TextureInstace> slide_end_texture;

  // 生成滑键
  void generate(Slide& slide) override;
};

#endif  // M_SLIDEGENERATOR_H
