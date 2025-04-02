#include "StaticRenderer.h"

StaticRenderer::StaticRenderer() { initializeOpenGLFunctions(); }

StaticRenderer::~StaticRenderer() {}
// 添加矩形
void StaticRenderer::addRect(const QRectF& rect, uint32_t textureId,
                             const QColor& fill_color) {}
// 添加椭圆
void StaticRenderer::addEllipse(const QRectF& bounds, uint32_t textureId,
                                const QColor& fill_color) {}
// 提交当前批次
void StaticRenderer::submitBatch() {}
