#ifndef MMM_MPAINTER_HPP
#define MMM_MPAINTER_HPP

#include <glm/glm.hpp>
#include <render/texture/TexMode.hpp>
#include <string>
#include <string_view>

class Renderer2D;

/**
 * @class MPainter
 * @brief 一个高级绘图接口，作为渲染指令的翻译器。
 */
class MPainter {
   public:
    // 构造MPainter
    explicit MPainter(Renderer2D* renderer2D);
    // 析构MPainter
    virtual ~MPainter();
    /**
     * @enum TextDirection
     * @brief 定义了文本的绘制方向。
     */
    enum class TextDirection {
        Horizontal,  ///< 水平方向，从左到右。
        Vertical     ///< 垂直方向，从上到下。
    };

    /**
     * @enum TileFitSide
     * @brief
     * 定义了在平铺（tileImage）时，单个图块（tile）如何根据目标区域进行缩放。
     */
    enum class TileFitSide {
        /**
         * @brief 无缩放。使用图像的原始尺寸作为图块大小进行平铺。
         * @note 这需要一个特定的着色器模式或CPU端的UV计算，因为它不直接映射到
         * TILE_BASEWIDTH_REPEAT 或 TILE_BASEHEIGHT_REPEAT。
         */
        NONE,
        /**
         * @brief 适应宽度。将图像缩放以匹配目标区域的宽度，高度等比缩放。
         *        然后使用这个缩放后的图像作为图块进行平铺。
         *        对应于着色器中的 `TexScaleMode::TILE_BASEWIDTH_REPEAT`。
         */
        WIDTH,
        /**
         * @brief 适应高度。将图像缩放以匹配目标区域的高度，宽度等比缩放。
         *        然后使用这个缩放后的图像作为图块进行平铺。
         *        对应于着色器中的 `TexScaleMode::TILE_BASEHEIGHT_REPEAT`。
         */
        HEIGHT,
    };

    /**
     * @brief 在指定位置绘制文本字符串。
     * @param fontFamily 字体族名称 (例如 "Arial", "Microsoft YaHei")。
     * @param fontSize 字体的大小
     * @param str 要绘制的UTF-32字符串。
     * @param pos 文本绘制的起始位置（通常是基线的左端点）。
     * @param direction 文本的排列方向（水平或垂直），默认为水平。
     * @param rotation 以弧度为单位旋转角度,围绕图像中心进行旋转,默认为 0.0f。
     * @param color 文本的颜色，默认为不透明白色。
     * @param applyMask 是否应用当前激活的蒙版效果。默认为 true。
     */
    void paintString(const std::string& fontFamily, uint32_t fontSize,
                     const std::u32string& str, glm::vec2 pos,
                     TextDirection direction = TextDirection::Horizontal,
                     float rotation = 0.0f, glm::vec4 color = glm::vec4(1.0f),
                     bool applyMask = true);

    /**
     * @brief 绘制一条线段。
     * @param pos1 线段的起点坐标。
     * @param pos2 线段的终点坐标。
     * @param color 线段的颜色。
     * @param lineWidth 线段的宽度（像素）。
     * @param applyMask 是否应用当前激活的蒙版效果。默认为 true。
     */
    void paintLine(glm::vec2 pos1, glm::vec2 pos2, glm::vec4 color,
                   float lineWidth, bool applyMask = true);

    /**
     * @brief 在指定位置以原始尺寸绘制图像。
     *
     * 此函数将图像的左上角放置在 `pos` 位置，不进行任何缩放。
     * 对应于着色器中的 `SINGLE` 策略配合左上角对齐。
     * @param resPath 图像的资源路径。
     * @param pos 图像左上角的绘制位置。
     * @param rotation 以弧度为单位旋转角度,围绕图像中心进行旋转,默认为 0.0f。
     * @param tint 应用于图像的色调，默认为白色（无效果）。
     * @param applyMask 是否应用当前激活的蒙版效果。默认为 true。
     */
    void paintImage(std::string_view resPath, glm::vec2 pos,
                    float rotation = 0.0f, glm::vec4 tint = glm::vec4(1.0f),
                    bool applyMask = true);

    /**
     * @brief 拉伸图像以强制填满指定的矩形区域。
     *
     * 此函数不保持图像的原始宽高比。
     * 是 `drawImage` 函数使用 `TexScaleMode::FORCE_FILL` 的一种便捷方式。
     * @param resPath 图像的资源路径。
     * @param pos 目标矩形区域的左上角位置。
     * @param size 目标矩形区域的尺寸（宽度和高度）。
     * @param rotation 以弧度为单位旋转角度,围绕图像中心进行旋转,默认为 0.0f。
     * @param tint 应用于图像的色调，默认为白色（无效果）。
     * @param applyMask 是否应用当前激活的蒙版效果。默认为 true。
     */
    void fillImage(std::string_view resPath, glm::vec2 pos, glm::vec2 size,
                   float rotation = 0.0f, glm::vec4 tint = glm::vec4(1.0f),
                   bool applyMask = true);

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
     * @param rotation 以弧度为单位旋转角度,围绕图像中心进行旋转,默认为 0.0f。
     * 定义第一个图块在目标区域内的对齐方式，后续的图块将基于此对齐点进行重复。默认为居中对齐。
     * @param tint 应用于图像的色调，默认为白色（无效果）。
     * @param applyMask 是否应用当前激活的蒙版效果。默认为 true。
     */
    void tileImage(std::string_view resPath, glm::vec2 pos, glm::vec2 size,
                   TileFitSide fitSide = TileFitSide::NONE,
                   TexAlignMode alignMode = TexAlignMode::CENTER,
                   float rotation = 0.0f, glm::vec4 tint = glm::vec4(1.0f),
                   bool applyMask = true);

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
     * @param rotation 以弧度为单位旋转角度,围绕图像中心进行旋转,默认为 0.0f。
     * @param tint 应用于图像的色调，默认为白色（无效果）。
     * @param applyMask 是否应用当前激活的蒙版效果。默认为 true。
     */
    void drawImage(std::string_view resPath, glm::vec2 pos, glm::vec2 size,
                   TexScaleMode scaleMode,
                   TexAlignMode alignMode = TexAlignMode::CENTER,
                   float rotation = 0.0f, glm::vec4 tint = glm::vec4(1.0f),
                   bool applyMask = true);

   private:
    Renderer2D* renderer;
};

#endif  // MMM_MPAINTER_HPP
