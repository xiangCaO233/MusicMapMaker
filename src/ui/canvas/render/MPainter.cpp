#include <glm/glm.hpp>
#include <render/MPainter.hpp>
#include <render/Renderer2D.hpp>
#include <string_view>

// 构造MPainter
MPainter::MPainter(Renderer2D* renderer2D) : renderer(renderer2D) {}
// 析构MPainter
MPainter::~MPainter() {
    // 析构时绘制
    renderer->finalize();
    renderer->render();
};

/**
 * @brief 在指定位置绘制文本字符串。
 * @param fontFamily 字体族名称 (例如 "Arial", "Microsoft YaHei")。
 * @param fontSize 字体的大小
 * @param str 要绘制的UTF-32字符串。
 * @param pos 文本绘制的起始位置（通常是基线的左端点）。
 * @param color 文本的颜色，默认为不透明白色。
 */
void MPainter::MPainter::paintString(const std::string& fontFamily,
                                     uint32_t fontSize,
                                     const std::u32string& str, glm::vec2 pos,
                                     TextDirection direction, float rotation,
                                     glm::vec4 color, bool applyMask) {}

/**
 * @brief 绘制一条线段。
 * @param pos1 线段的起点坐标。
 * @param pos2 线段的终点坐标。
 * @param color 线段的颜色。
 * @param lineWidth 线段的宽度（像素）。
 */
void MPainter::paintLine(glm::vec2 pos1, glm::vec2 pos2, glm::vec4 color,
                         float lineWidth, bool applyMask) {
    // 步骤 1: 计算线段向量和长度 (矩形的宽度)
    glm::vec2 delta = pos2 - pos1;
    float length = glm::length(delta);
    // 如果线段长度为0，则无需绘制
    if (length <= 0.0f) {
        return;
    }
    // 计算旋转角度 (弧度)
    // atan2(y, x) 返回向量(x, y)与正X轴之间的角度
    float rotation = std::atan2(delta.y, delta.x);
    // 确定矩形的尺寸
    auto size = glm::vec2(length, lineWidth);
    // 计算线段中点 (即矩形的中心点)
    glm::vec2 center = (pos1 + pos2) / 2.0f;
    // 根据中心点和尺寸计算矩形左上角的位置
    glm::vec2 pos = center - (size / 2.0f);
    // 准备提交给渲染器的指令
    // 创建一个空纹理信息
    TextureInfo texture{};

    // 提交渲染指令
    // 注意：这里的 color 应该用函数的参数 color，而不是 tint。我们重命名一下。
    renderer->commit({pos, size, rotation, color, texture, !applyMask,
                      TexAlignMode::CENTER, TexScaleMode::SINGLE});
}

/**
 * @brief 在指定位置以原始尺寸绘制图像。
 *
 * 此函数将图像的左上角放置在 `pos` 位置，不进行任何缩放。
 * 对应于着色器中的 `SINGLE` 策略配合左上角对齐。
 * @param resPath 图像的资源路径。
 * @param pos 图像左上角的绘制位置。
 * @param tint 应用于图像的色调，默认为白色（无效果）。
 */
void MPainter::paintImage(std::string_view resPath, glm::vec2 pos,
                          float rotation, glm::vec4 tint, bool applyMask) {
    auto texoption = renderer->texture_pool()->get(std::string(resPath));
    if (texoption.has_value()) {
        const auto& texture = texoption.value();
        renderer->commit({pos, texture.origin_size, rotation, tint, texture,
                          !applyMask, TexAlignMode::CENTER,
                          TexScaleMode::SINGLE});
    }
}

/**
 * @brief 拉伸图像以强制填满指定的矩形区域。
 *
 * 此函数不保持图像的原始宽高比。
 * 是 `drawImage` 函数使用 `TexScaleMode::FORCE_FILL` 的一种便捷方式。
 * @param resPath 图像的资源路径。
 * @param pos 目标矩形区域的左上角位置。
 * @param size 目标矩形区域的尺寸（宽度和高度）。
 * @param tint 应用于图像的色调，默认为白色（无效果）。
 */
void MPainter::fillImage(std::string_view resPath, glm::vec2 pos,
                         glm::vec2 size, float rotation, glm::vec4 tint,
                         bool applyMask) {
    drawImage(resPath, pos, size, TexScaleMode::FORCE_FILL,
              TexAlignMode::CENTER, rotation, tint, applyMask);
}

/**
 * @brief 在指定区域内平铺绘制图像。
 *
 * 此函数通过重复一个图像（作为图块）来填充一个矩形区域。
 * 它是对核心函数 drawImage() 在平铺场景下的一个便捷封装。
 *
 * @param resPath 图像的资源路径。
 * @param pos 目标矩形区域的左上角位置。
 * @param size 目标矩形区域的总尺寸（宽度和高度）。
 * @param fitSide
 * 定义单个图块的缩放方式。决定了是使用原始图像平铺，还是使用缩放后的图像平铺。
 * @param alignMode
 * 定义第一个图块在目标区域内的对齐方式，后续的图块将基于此对齐点进行重复。默认为居中对齐。
 * @param tint 应用于图像的色调，默认为白色（无效果）。
 */
void MPainter::tileImage(std::string_view resPath, glm::vec2 pos,
                         glm::vec2 size, MPainter::TileFitSide fitSide,
                         TexAlignMode alignMode, float rotation, glm::vec4 tint,
                         bool applyMask) {
    TexScaleMode scalemode;
    switch (fitSide) {
        using enum MPainter::TileFitSide;
        using enum TexScaleMode;
        case NONE: {
            scalemode = TILE_REPEAT;
            break;
        }
        case WIDTH: {
            scalemode = TILE_BASEWIDTH_REPEAT;
            break;
        }
        case HEIGHT: {
            scalemode = TILE_BASEHEIGHT_REPEAT;
            break;
        }
    }
    drawImage(resPath, pos, size, scalemode, alignMode, rotation, tint,
              applyMask);
}

/**
 * @brief 根据指定的策略在矩形区域内绘制图像。
 *
 * 允许完全控制图像的缩放、平铺和对齐方式。
 *
 * @param resPath 图像的资源路径。
 * @param pos 目标矩形区域的左上角位置。
 * @param size 目标矩形区域的尺寸（宽度和高度）。
 * @param scaleMode 纹理的缩放/平铺策略，定义了图像如何适应 `size`。
 * @param alignMode 对齐策略，当 `scaleMode` 导致有留白时生效。
 * @param tint 应用于图像的色调，默认为白色（无效果）。
 */
void MPainter::drawImage(std::string_view resPath, glm::vec2 pos,
                         glm::vec2 size, TexScaleMode scaleMode,
                         TexAlignMode alignMode, float rotation, glm::vec4 tint,
                         bool applyMask) {
    auto texoption = renderer->texture_pool()->get(std::string(resPath));
    if (texoption.has_value()) {
        const auto& texture = texoption.value();
        // struct RenderCommand {
        //     glm::vec2 pos;
        //     glm::vec2 size;
        //     glm::f32 rotation;
        //     glm::vec4 color;
        //     TextureInfo texture;
        //     glm::uint32 no_filter;
        //     TexAlignMode talign;
        //     TexScaleMode tscale;
        // }
        renderer->commit({pos, size, rotation, tint, texture, !applyMask,
                          alignMode, scaleMode});
    }
}
