#include "GLRenderer.h"

GLRenderer::GLRenderer() {}

GLRenderer::~GLRenderer() {}
// 添加矩形
void GLRenderer::addRect(const QRectF& rect, uint32_t textureId,
                         const QColor& fill_color) {}
// 添加椭圆
void GLRenderer::addEllipse(const QRectF& bounds, uint32_t textureId,
                            const QColor& fill_color) {}
// 提交当前批次
void GLRenderer::submitBatch() {}
