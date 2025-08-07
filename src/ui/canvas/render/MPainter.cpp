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
 * @brief 绘制一条线段，支持圆角。
 * @param pos1 线段的起点坐标。
 * @param pos2 线段的终点坐标。
 * @param color 线段的颜色。
 * @param lineWidth 线段的宽度（像素）。
 * @param radiusInfo (可选) 描述线段（矩形）的圆角效果。
 * @param applyMask (可选) 是否应用当前激活的蒙版效果。
 */
void MPainter::paintLine(glm::vec2 pos1, glm::vec2 pos2, glm::vec4 color,
                         float lineWidth, const RadiusInfo& radiusInfo,
                         bool applyMask) {
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
    renderer->commit({{pos, size, rotation, color, !applyMask},
                      radiusInfo,
                      {texture, TexAlignMode::CENTER, TexScaleMode::SINGLE}});
}

/**
 * @brief [核心] 根据指定的策略在矩形区域内绘制图像，支持圆角。
 * @param resPath 图像的资源路径。
 * @param rectOpts 描述矩形的位置、尺寸、旋转和颜色。
 * @param mapOpts (可选) 描述纹理的缩放和对齐方式。
 * @param radiusInfo (可选) 描述矩形的圆角效果。
 */
void MPainter::drawImage(std::string_view resPath, const RectOptions& rectOpts,
                         const TextureMapOptions& mapOpts,
                         const RadiusInfo& radiusInfo) {
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
        renderer->commit({{rectOpts.pos, rectOpts.size, rectOpts.rotation,
                           rectOpts.color, !rectOpts.applyMask},
                          radiusInfo,
                          {texture, mapOpts.alignMode, mapOpts.scaleMode}});
    }
}

/**
 * @brief 在指定位置以原始尺寸绘制图像，支持圆角。
 * @param resPath 图像的资源路径。
 * @param pos 图像左上角的绘制位置。
 * @param radiusInfo (可选) 描述矩形的圆角效果。
 * @param tint (可选) 应用于图像的色调。
 * @param rotation (可选) 旋转角度。
 */
void MPainter::paintImage(std::string_view resPath, glm::vec2 pos,
                          const RadiusInfo& radiusInfo, glm::vec4 tint,
                          float rotation, bool applyMask) {
    auto texoption = renderer->texture_pool()->get(std::string(resPath));
    if (texoption.has_value()) {
        const auto& texture = texoption.value();
        renderer->commit(
            {{pos, texture.origin_size, rotation, tint, !applyMask},
             radiusInfo,
             {texture, TexAlignMode::CENTER, TexScaleMode::SINGLE}});
    }
}

/**
 * @brief 拉伸图像以强制填满指定的矩形区域，支持圆角。
 * @param resPath 图像的资源路径。
 * @param rectOpts 描述矩形的位置、尺寸、旋转和颜色。
 * @param radiusInfo (可选) 描述矩形的圆角效果。
 */
void MPainter::fillImage(std::string_view resPath, const RectOptions& rectOpts,
                         const RadiusInfo& radiusInfo) {
    TextureMapOptions mapOpts{TexScaleMode::FORCE_FILL, TexAlignMode::CENTER};
    drawImage(resPath, rectOpts, mapOpts, radiusInfo);
}

/**
 * @brief 在指定区域内平铺绘制图像，支持圆角。
 * @param resPath 图像的资源路径。
 * @param rectOpts 描述矩形的位置、尺寸、旋转和颜色。
 * @param fitSide 定义单个图块的缩放方式。
 * @param alignMode (可选) 定义第一个图块的对齐方式。
 * @param radiusInfo (可选) 描述矩形的圆角效果。
 */
void MPainter::tileImage(std::string_view resPath, const RectOptions& rectOpts,
                         TileFitSide fitSide, TexAlignMode alignMode,
                         const RadiusInfo& radiusInfo) {
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
    TextureMapOptions mapOpts{scalemode, alignMode};
    drawImage(resPath, rectOpts, mapOpts, radiusInfo);
}
