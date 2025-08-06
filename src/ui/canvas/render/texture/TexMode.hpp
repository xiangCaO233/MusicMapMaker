#ifndef MMM_TEXMODE_HPP
#define MMM_TEXMODE_HPP

// 纹理缩放模式

#include <cstdint>

enum class TexScaleMode : uint32_t {
    // 自动根据缩放并平铺(确保填满矩形,并保持比例的缩放,始终对齐中心)
    TILING_AUTO = 0x00000001,
    // 强行根据宽度缩放并平铺(确保填满矩形宽度(不确保填满矩形),基于此宽度的缩放保持图像比例等比缩放高度)
    SCALE_BASEWIDTH_TO_TILING = 0x00000002,
    // 强行根据高度缩放并平铺(确保填满矩形高度(不确保填满矩形),基于此高度的缩放保持图像比例等比缩放宽度)
    SCALE_BASEHEIGHT_TO_TILING = 0x00000003,
    // 缩放并直接填充(不保持比例,直接用图像塞满矩形)
    SCALE_FILL = 0x00000004,
    // 缩放的填充(确保填满矩形宽度,保持比例,不够的高度使用重采样,确保最终填满矩形)
    SCALE_BASEWIDTH_REPEAT_FILL = 0x00000005,
    // (确保填满矩形高度,保持比例,不够的宽度使用重采样,确保最终填满矩形)
    SCALE_BASEHEIGHT_REPEAT_FILL = 0x00000006,
    // 不缩放的平铺(保持比例,根据对齐方式铺过去,不够则不填充颜色(无重采样),图像过大则应只显示一部分,需要配合对齐方式指定对齐位置)
    NO_SCALE_TILING = 0x00000007,
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
