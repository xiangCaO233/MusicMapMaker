#include "BaseTexturePool.h"

BaseTexturePool::BaseTexturePool() {
  // 初始化单白像素点
}

BaseTexturePool::~BaseTexturePool() = default;

// 载入纹理
void BaseTexturePool::load_texture(const char* resource_path) {}

// 使用此纹理池
void BaseTexturePool::bind() {}
