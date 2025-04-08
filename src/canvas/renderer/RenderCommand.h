#ifndef RENDERCOMMAND_H
#define RENDERCOMMAND_H

#include <qcolor.h>
#include <qrect.h>

#include <sstream>
#include <string>

#include "../texture/Texture.h"

// 形状
enum class ShapeType { QUAD, OVAL };

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

  // 纹理
  std::shared_ptr<TextureInstace> texture;

  // 纹理对齐模式
  TextureAlignMode texture_alignmode;
  // 纹理填充模式
  TextureFillMode texture_fillmode;
  // 纹理补充模式
  TextureComplementMode texture_complementmode;

  // 重写==运算符
  bool operator==(const RenderCommand& other) const {
    return is_volatile == other.is_volatile &&
           instance_shape == other.instance_shape &&
           instace_bound == other.instace_bound &&
           fill_color == other.fill_color && texture == other.texture;
  }
  std::string toString() const {
    std::ostringstream oss;

    oss << "RenderCommand {"
        << "\n  is_volatile: " << (is_volatile ? "true" : "false")
        << "\n  instance_shape: ";

    // Handle ShapeType enum
    switch (instance_shape) {
      case ShapeType::QUAD:
        oss << "QUAD";
        break;
      case ShapeType::OVAL:
        oss << "OVAL";
        break;
    }

    oss << "\n  instace_bound: "
        << "QRectF(" << instace_bound.x() << ", " << instace_bound.y() << ", "
        << instace_bound.width() << ", " << instace_bound.height() << ")"
        << "\n  rotation: " << rotation << "°"
        << "\n  fill_color: " << fill_color.name().toStdString()
        << "\n  texture: ";

    if (texture) {
      oss << texture->name;
    } else {
      oss << "nullptr";
    }

    // Handle TextureAlignMode enum
    oss << "\n  texture_alignmode: ";
    switch (texture_alignmode) {
      case TextureAlignMode::ALIGN_TO_LEFT_BOTTOM:
        oss << "ALIGN_TO_LEFT_BOTTOM";
        break;
      case TextureAlignMode::ALIGN_TO_RIGHT_BOTTOM:
        oss << "ALIGN_TO_RIGHT_BOTTOM";
        break;
      case TextureAlignMode::ALIGN_TO_LEFT_TOP:
        oss << "ALIGN_TO_LEFT_TOP";
        break;
      case TextureAlignMode::ALIGN_TO_RIGHT_TOP:
        oss << "ALIGN_TO_RIGHT_TOP";
        break;
      case TextureAlignMode::ALIGN_TO_CENTER:
        oss << "ALIGN_TO_CENTER";
        break;
    }

    // Handle TextureFillMode enum
    oss << "\n  texture_fillmode: ";
    switch (texture_fillmode) {
      case TextureFillMode::FILL:
        oss << "SCALLING";
        break;
      case TextureFillMode::KEEP:
        oss << "KEEP";
        break;
      case TextureFillMode::SCALLING_AND_TILE:
        oss << "SCALLING_AND_TILE";
        break;
      case TextureFillMode::SCALLING_BASE_WIDTH_AND_CUT:
        oss << "SCALLING_BASE_WIDTH_AND_CUT";
        break;
      case TextureFillMode::SCALLING_BASE_HEIGHT_AND_CUT:
        oss << "SCALLING_BASE_HEIGHT_AND_CUT";
        break;
    }

    // Handle TextureComplementMode enum
    oss << "\n  texture_complementmode: ";
    switch (texture_complementmode) {
      case TextureComplementMode::FILL_COLOR:
        oss << "FILL_COLOR";
        break;
      case TextureComplementMode::REPEAT_TEXTURE:
        oss << "REPEAT_TEXTURE";
        break;
    }

    oss << "\n}";

    return oss.str();
  }
};

#endif  // RENDERCOMMAND_H
