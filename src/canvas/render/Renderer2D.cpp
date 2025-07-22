#include <canvas/render/Renderer2D.hpp>
#include <cstdint>

struct QuadVertex {
    QVector3D Position;
    QColor Color;
    // uv
    QVector2D TexCoord;
    // 图集ID或独立纹理的标识
    float TexIndex;
    // 用于在Shader中实现平铺
    float TilingFactor;
    // 用于索引采样模式等材质属性
    int MaterialID;
};

struct Renderer2DData {
    // 渲染状态
    QMatrix4x4 viewProjectionMatrix;

    // 批处理所需资源
    uint32_t quadVao = 0;
    uint32_t quadVbo = 0;
    uint32_t quadIbo = 0;

    // 基本矩形着色器
    std::shared_ptr<Shader> quadShader;

    // CPU端顶点缓冲区
    const uint32_t MaxQuads = 32768;
    const uint32_t MaxVertices = MaxQuads * 4;
    const uint32_t MaxIndices = MaxQuads * 6;

    std::vector<QuadVertex> quadVertexBuffer;

    // 当前可写位置
    QuadVertex* availableQuadVertexBufferPtr = nullptr;

    // 纹理管理
    // 用于绘制纯色Quad
    std::shared_ptr<Texture> whiteTexture;
    // 当前批次使用的纹理
    std::vector<std::shared_ptr<Texture>> textureSlots;
    // 0号槽纯白纹理
    uint32_t textureSlotIndex = 1;

    // 统计
    Renderer2D::Statistics stats;
};

// 静态变量
Renderer2D::Renderer2DData* Renderer2D::data;
Renderer2D::Statistics Renderer2D::stats;

// 生命周期
void Renderer2D::Init() {}

void Renderer2D::Shutdown() {}

// 渲染流程控制
void Renderer2D::BeginScene() {}
void Renderer2D::EndScene() {}

// 绘制命令

// 绘制带纯色的Quad
void Renderer2D::DrawQuad(const QVector2D& position, const QVector2D& size,
                          const QColor& color, float z) {}

void Renderer2D::DrawQuad(const QMatrix4x4& transform, const QColor& color) {}

// 绘制带纹理的Quad
void Renderer2D::DrawQuad(const QVector2D& position, const QVector2D& size,
                          const std::shared_ptr<Texture>& texture, float z,
                          const QColor& tintColor,
                          const QVector2D& tilingFactor) {}

void Renderer2D::DrawQuad(const QMatrix4x4& transform,
                          const std::shared_ptr<Texture>& texture,
                          const QColor& tintColor,
                          const QVector2D& tilingFactor) {}

// 绘制文本
void Renderer2D::DrawString(const QString& text,
                            const std::shared_ptr<Font>& font,
                            const QMatrix4x4& transform, const QColor& color,
                            float kerning, float lineSpacing) {}

Renderer2D::Statistics Renderer2D::GetStats() { return stats; }

void Renderer2D::ResetStats() {}
