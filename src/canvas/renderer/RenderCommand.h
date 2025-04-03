#ifndef RENDERCOMMAND_H
#define RENDERCOMMAND_H

#include <qcolor.h>
#include <qrect.h>

#include <cstdint>

// 形状
enum class ShapeType { QUAD, OVAL };

// 纹理对齐模式
enum class TextureAlignMode : int16_t {
  // 对齐左下角
  ALIGN_TO_LEFT_BOTTOM = 0x10,
  // 对齐右下角
  ALIGN_TO_RIGHT_BOTTOM = 0x20,
  // 对齐左上角
  ALIGN_TO_LEFT_TOP = 0x30,
  // 对齐右上角
  ALIGN_TO_RIGHT_TOP = 0x40,
  // 对齐中心
  ALIGN_TO_RIGHT_CENTER = 0x50,
};

// 纹理填充模式
enum class TextureFillMode : int16_t {
  // 缩放
  SCALLING = 0x01,
  // 缩放并裁切
  SCALLING_AND_CUT = 0x02,
  // 缩放并保持比例
  SCALLING_AND_KEEP_RATIO = 0x03,
  // 平铺
  TILE = 0x04,
};

struct TextureInstace;

// 纹理信息
struct TextureInfo {
  // 纹理(-1时仅填充颜色)
  TextureInstace* texture_instance;
  // 纹理对齐模式
  TextureAlignMode texture_alignmode;
  // 纹理填充模式
  TextureFillMode texture_fillmode;
};

// 绘制指令
struct RenderCommand {
  // 渲染内容是否易变
  bool is_volatile;
  // 绘制的图元形状
  ShapeType instance_shape;
  // 图元边界矩形的位置
  QRectF instace_bound;
  // 图元旋转角度
  float rotation;
  // 填充颜色(无绑定纹理时或被裁切部分的填充颜色)
  QColor fill_color;
  // 纹理信息
  TextureInfo* texture_info;

  // 重写==运算符
  bool operator==(const RenderCommand& other) const {
    return is_volatile == other.is_volatile &&
           instance_shape == other.instance_shape &&
           instace_bound == other.instace_bound &&
           fill_color == other.fill_color && texture_info == other.texture_info;
  }
};

#endif  // RENDERCOMMAND_H
