#ifndef RENDERCOMMAND_H
#define RENDERCOMMAND_H

#include <qcolor.h>
#include <qrect.h>

#include <cstdint>
#include <sstream>
#include <string>

#include "../texture/Texture.h"

// 形状
enum class ShapeType {
    RECT,
    TEXT,
    OVAL,
};

struct Text {
    // 字体名
    std::string font_family;
    // 字体像素尺寸
    uint32_t font_size;
    // unicode字符
    char32_t character;
    // 终止标识
    bool is_string_ending;

    bool operator==(const Text& other) const {
        return font_size == other.font_size && character == other.character &&
               is_string_ending == other.is_string_ending;
    }
};

// 绘制指令
struct RenderCommand {
    // 是否为文本渲染
    bool is_text;
    // 渲染的文本
    Text text;
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
    // 圆角半径
    float radius;

    // 纹理
    std::shared_ptr<TextureInstace> texture;

    // 纹理特效
    TextureEffect texture_effect;
    // 纹理对齐模式
    TextureAlignMode texture_alignmode;
    // 纹理填充模式
    TextureFillMode texture_fillmode;
    // 纹理补充模式
    TextureComplementMode texture_complementmode;

    // 重写==运算符
    bool operator==(const RenderCommand& other) const {
        return is_text == other.is_text && text == other.text &&
               is_volatile == other.is_volatile &&
               instance_shape == other.instance_shape &&
               instace_bound == other.instace_bound &&
               rotation == other.rotation && fill_color == other.fill_color &&
               texture == other.texture &&
               texture_effect == other.texture_effect &&
               texture_alignmode == other.texture_alignmode &&
               texture_fillmode == other.texture_fillmode &&
               texture_complementmode == other.texture_complementmode;
    }
};

#endif  // RENDERCOMMAND_H
