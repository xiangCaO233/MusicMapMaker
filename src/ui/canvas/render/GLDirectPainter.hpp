#ifndef MMM_MPAINTER_HPP
#define MMM_MPAINTER_HPP

#include <glm/glm.hpp>
#include <render/RenderCommand.hpp>
#include <render/texture/TexMode.hpp>
#include <string>
#include <string_view>

class Renderer2D;

/**
 * @class GLDirectPainter
 * @brief 一个直接绘图接口,作为渲染指令的翻译器,在释放时直接执行渲染
 */
class GLDirectPainter {
   public:
    // 构造MPainter
    explicit GLDirectPainter(Renderer2D* renderer2D);
    // 析构MPainter
    virtual ~GLDirectPainter();
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
     * @struct RectOptions
     * @brief 封装了绘制一个矩形所需的所有几何与样式属性。
     */
    struct RectOptions {
        glm::vec2 pos;                      ///< 矩形左上角的位置。
        glm::vec2 size;                     ///< 矩形的尺寸。
        float rotation = 0.0f;              ///< 旋转角度 (弧度)，围绕矩形中心。
        glm::vec4 color = glm::vec4(1.0f);  ///< 颜色或应用于纹理的色调。
        bool applyMask = true;              ///< 是否应用蒙版。
    };

    /**
     * @struct TextureMapOptions
     * @brief 封装了纹理贴图相关的策略。
     */
    struct TextureMapOptions {
        ///< 纹理缩放策略。
        TexScaleMode scaleMode = TexScaleMode::AUTO_SCALE_AND_CUT;
        ///< 纹理对齐策略。
        TexAlignMode alignMode = TexAlignMode::CENTER;
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
                     glm::vec4 color = glm::vec4(1.0f),
                     TextDirection direction = TextDirection::Horizontal,
                     float rotation = 0.0f, bool applyMask = true);

    /**
     * @brief 绘制一条线段，支持圆角。
     * @param pos1 线段的起点坐标。
     * @param pos2 线段的终点坐标。
     * @param color 线段的颜色。
     * @param lineWidth 线段的宽度（像素）。
     * @param radiusInfo (可选) 描述线段（矩形）的圆角效果。
     * @param applyMask (可选) 是否应用当前激活的蒙版效果。
     */
    void paintLine(glm::vec2 pos1, glm::vec2 pos2, glm::vec4 color,
                   float lineWidth, const RadiusInfo& radiusInfo = {},
                   bool applyMask = true);

    /**
     * @brief [核心] 根据指定的策略在矩形区域内绘制图像，支持圆角。
     * @param resPath 图像的资源路径。
     * @param rectOpts 描述矩形的位置、尺寸、旋转和颜色。
     * @param mapOpts (可选) 描述纹理的缩放和对齐方式。
     * @param radiusInfo (可选) 描述矩形的圆角效果。
     */
    void drawImage(std::string_view resPath, const RectOptions& rectOpts,
                   const TextureMapOptions& mapOpts =
                       {TexScaleMode::AUTO_SCALE_AND_CUT, TexAlignMode::CENTER},
                   const RadiusInfo& radiusInfo = {});

    /**
     * @brief 在指定位置以原始尺寸绘制图像，支持圆角。
     * @param resPath 图像的资源路径。
     * @param pos 图像左上角的绘制位置。
     * @param radiusInfo (可选) 描述矩形的圆角效果。
     * @param tint (可选) 应用于图像的色调。
     * @param rotation (可选) 旋转角度。
     */
    void paintImage(std::string_view resPath, glm::vec2 pos,
                    const RadiusInfo& radiusInfo = {},
                    glm::vec4 tint = glm::vec4(1.0f), float rotation = 0.0f,
                    bool applyMask = true);

    /**
     * @brief 拉伸图像以强制填满指定的矩形区域，支持圆角。
     * @param resPath 图像的资源路径。
     * @param rectOpts 描述矩形的位置、尺寸、旋转和颜色。
     * @param radiusInfo (可选) 描述矩形的圆角效果。
     */
    void fillImage(std::string_view resPath, const RectOptions& rectOpts,
                   const RadiusInfo& radiusInfo = {});

    /**
     * @brief 在指定区域内平铺绘制图像，支持圆角。
     * @param resPath 图像的资源路径。
     * @param rectOpts 描述矩形的位置、尺寸、旋转和颜色。
     * @param fitSide 定义单个图块的缩放方式。
     * @param alignMode (可选) 定义第一个图块的对齐方式。
     * @param radiusInfo (可选) 描述矩形的圆角效果。
     */
    void tileImage(std::string_view resPath, const RectOptions& rectOpts,
                   TileFitSide fitSide,
                   TexAlignMode alignMode = TexAlignMode::CENTER,
                   const RadiusInfo& radiusInfo = {});

   private:
    Renderer2D* renderer;
};

#endif  // MMM_MPAINTER_HPP
