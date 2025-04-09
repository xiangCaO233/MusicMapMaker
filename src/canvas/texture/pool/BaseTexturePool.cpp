#include "BaseTexturePool.h"

#include "colorful-log.h"

BaseTexturePool::BaseTexturePool(GLCanvas* canvas) : cvs(canvas) {}

BaseTexturePool::~BaseTexturePool() = default;

// 获取纹理指针
std::shared_ptr<TextureInstace> BaseTexturePool::get_texture(
    const std::string& name) {
  auto it = texture_map.find(name);
  if (it == texture_map.end()) {
    XWARN("不存在纹理[" + name + "]");
    return nullptr;
  }
  return it->second;
}
