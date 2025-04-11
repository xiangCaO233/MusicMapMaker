#include "BaseTexturePool.h"

#include "../atlas/TextureAtlas.h"
#include "colorful-log.h"

// 用于包装 OpenGL 调用并检查错误
#define GLCALL(func)                                       \
  func;                                                    \
  {                                                        \
    XLogger::glcalls++;                                    \
    GLenum error = cvs->glGetError();                      \
    if (error != GL_NO_ERROR) {                            \
      XERROR("在[" + std::string(#func) +                  \
             "]发生OpenGL错误: " + std::to_string(error)); \
    }                                                      \
  }

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
// 上传纹理集元数据组
void BaseTexturePool::upload_atlas_data() {
  // 初始化纹理数组存储的纹理集数据组
  GLCALL(cvs->glGenTextures(1, &atlasMetaTextureArray));
  // 激活纹理单元
  GLCALL(cvs->glActiveTexture(GL_TEXTURE0 + 14));
  GLCALL(cvs->glBindTexture(GL_TEXTURE_2D_ARRAY, atlasMetaTextureArray));
  // 设置纹理参数
  GLCALL(cvs->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER,
                              GL_NEAREST));
  GLCALL(cvs->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER,
                              GL_NEAREST));
  GLCALL(cvs->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S,
                              GL_CLAMP_TO_EDGE));
  GLCALL(cvs->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T,
                              GL_CLAMP_TO_EDGE));
}
