#ifndef MMM_TEXMODE_HPP
#define MMM_TEXMODE_HPP

// 纹理缩放模式

#include <cstdint>

enum class TexScaleMode : uint32_t {
    // 缩放并平铺
    SCALE_TO_TILING = 0x00000001,
    // 不缩放的平铺
    NO_SCALE_TILING = 0x00000002,
    // 无视比例缩放到矩形的尺寸
    SCALE_TO_QUAD = 0x00000003,
    // 保持纹理尺寸
    KEEP_TEXSCALE = 0x00000004,
    // 直接填充
    FILL = 0x00000005,
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
