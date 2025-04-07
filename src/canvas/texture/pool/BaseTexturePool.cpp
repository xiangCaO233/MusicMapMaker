#include "BaseTexturePool.h"

BaseTexturePool::BaseTexturePool(GLCanvas* canvas) : cvs(canvas) {
  // 初始化单白像素点
}

BaseTexturePool::~BaseTexturePool() = default;

// 获取纹理指针
std::shared_ptr<TextureInstace> get_texture(const std::string& name);
