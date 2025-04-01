#include "RenderCommand.h"

#include <qcolor.h>

#include <cstdint>
#include <glm/vec3.hpp>

// 形状
enum class Shape { TRIANGLE, QUAD, OVAL };

// 纹理对齐模式
enum class TextureAlignMode {
  // 对齐左下角
  ALIGN_TO_LEFT_BOTTOM,
  // 对齐右下角
  ALIGN_TO_RIGHT_BOTTOM,
  // 对齐左上角
  ALIGN_TO_LEFT_TOP,
  // 对齐右上角
  ALIGN_TO_RIGHT_TOP,
  // 对齐中心
  ALIGN_TO_RIGHT_CENTER,
};

// 纹理填充模式
enum class TextureFillMode {
  // 缩放
  SCALLING,
  // 缩放并裁切
  SCALLING_AND_CUT,
  // 缩放并保持比例
  SCALLING_AND_KEEP_RATIO,
  // 平铺
  TILE,
};

struct RenderCommand {
  // 绘制的图元形状
  Shape instance_shape;
  // 纹理
  int32_t texture_id{-1};
  TextureAlignMode texture_alignmode;
  TextureFillMode texture_fillmode;
  // 填充颜色(无绑定纹理时或被裁切部分的填充颜色)
  QColor fill_color;
};
