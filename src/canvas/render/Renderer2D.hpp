#ifndef MMM_RENDERER2D_HPP
#define MMM_RENDERER2D_HPP

#include <qcolor.h>
#include <qvectornd.h>

#include <QMatrix4x4>

class Texture;
class Font;
class Shader;

class Renderer2D {
   public:
    // 生命周期
    static void Init();
    static void Shutdown();

    // 渲染流程控制
    static void BeginScene();
    static void EndScene();

    // 2D 绘制命令

    // 绘制带纯色的Quad
    static void DrawQuad(const QVector2D& position, const QVector2D& size,
                         const QColor& color, float z = 0.0f);
    static void DrawQuad(const QMatrix4x4& transform, const QColor& color);

    // 绘制带纹理的Quad
    static void DrawQuad(const QVector2D& position, const QVector2D& size,
                         const std::shared_ptr<Texture>& texture,
                         float z = 0.0f, const QColor& tintColor = Qt::white,
                         const QVector2D& tilingFactor = {1.0f, 1.0f});
    static void DrawQuad(const QMatrix4x4& transform,
                         const std::shared_ptr<Texture>& texture,
                         const QColor& tintColor = Qt::white,
                         const QVector2D& tilingFactor = {1.0f, 1.0f});

    // 绘制文本
    static void DrawString(const QString& text,
                           const std::shared_ptr<Font>& font,
                           const QMatrix4x4& transform, const QColor& color,
                           float kerning = 0.0f, float lineSpacing = 0.0f);

    // 统计信息
    struct Statistics {
        uint32_t drawCalls = 0;
        uint32_t quadCount = 0;
    };

    static Statistics GetStats();
    static void ResetStats();

   private:
    struct Renderer2DData;
    static Renderer2DData* data;
    static Statistics stats;
};

#endif  // MMM_RENDERER2D_HPP
