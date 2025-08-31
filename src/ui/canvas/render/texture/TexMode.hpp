#ifndef MMM_TEXMODE_HPP
#define MMM_TEXMODE_HPP

// 纹理缩放模式

#include <cstdint>

enum class TexScaleMode : uint32_t {
    // 自动缩放并裁切(确保填满矩形,并保持比例的缩放,始终对齐中心)
    AUTO_SCALE_AND_CUT = 0x00000001,
    // 强行根据宽度缩放(确保填满矩形宽度(不确保填满矩形),基于此宽度的缩放保持图像比例等比缩放高度)
    SCALE_FORCE_BASEWIDTH = 0x00000002,
    // 强行根据高度缩放(确保填满矩形高度(不确保填满矩形),基于此高度的缩放保持图像比例等比缩放宽度)
    SCALE_FORCE_BASEHEIGHT = 0x00000003,
    // 强制填充(不保持比例,直接用图像塞满矩形)
    FORCE_FILL = 0x00000004,
    // 直接平铺(确保填满矩形宽度,保持比例,保持尺寸,不够的高度使用重采样,确保最终填满矩形)
    TILE_REPEAT = 0x00000005,
    // 根据宽度平铺(确保填满矩形宽度,保持比例,不够的高度使用重采样,确保最终填满矩形)
    TILE_BASEWIDTH_REPEAT = 0x00000006,
    // 根据高度平铺(确保填满矩形高度,保持比例,不够的宽度使用重采样,确保最终填满矩形)
    TILE_BASEHEIGHT_REPEAT = 0x00000007,
    // 单独放一个图像(保持比例,根据对齐方式铺过去,不够则不填充颜色(无重采样),图像过大则应只显示一部分,需要配合对齐方式指定对齐位置)
    SINGLE = 0x00000008,
    // 仅绘制字符-此情况下圆角半径属性存储的是uvOffset
    CHARACTER = 0x00000009,
};

// 纹理对齐方式
enum class TexAlignMode : uint32_t {
    // 对齐位置
    CENTER = 0x00000010,
    LEFT = 0x00000020,
    RIGHT = 0x00000030,
    TOP = 0x00000040,
    BOTTOM = 0x00000050,
};

// 圆角效果
enum class RadiusEffect : uint32_t {
    // 淡入淡出
    FADE_IN_AND_OUT = 0x00000001,
    // 发光
    GLOWING = 0x00000002,
};

// 纹理的蒙版效果
enum class MaskEffect : uint32_t {
    // 无效果
    NONE = 0x00000001,
    // 暗化
    DARKEN = 0x00000002,
    // 滤镜
    FILTER = 0x00000003,
    // 透明变换
    ALPHA_SHIFT = 0x00000004,
};

#endif  // MMM_TEXMODE_HPP
