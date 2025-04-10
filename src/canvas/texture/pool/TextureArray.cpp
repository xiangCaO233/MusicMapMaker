#include "TextureArray.h"

#include <cmath>
#include <cstdint>

#include "../../../log/colorful-log.h"
#include "../../GLCanvas.h"

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

// 最大采样器层数
uint32_t TextureArray::max_texture_layer = 0;

TextureArray::TextureArray(GLCanvas* canvas, QSize size)
    : BaseTexturePool(canvas), texture_size(size) {
  XINFO("构造纹理数组纹理池");
  pool_type = TexturePoolType::ARRAY;
}

TextureArray::~TextureArray() {}

// 判满
bool TextureArray::is_full() { return texture_map.size() >= max_texture_layer; }

// 载入纹理
int TextureArray::load_texture(std::shared_ptr<TextureInstace> texture) {
  if (is_full()) return POOL_FULL;
  if (texture->texture_image.size() != texture_size) return EXPECTED_SIZE;
  if (texture_array.empty()) {
    first_texture_id_offset = current_id_handle;
  }
  if (is_consecutive()) {
    texture_array.emplace_back(texture);
    texture_map.try_emplace(texture->name, texture);
    texture->texture_id = current_id_handle++;
    XINFO("添加纹理: " + texture->name + "成功");
    return SUCCESS;
  } else {
    return NOT_CONSECUTIVE;
  }
}

// 检查纹理id是否与当前纹理数组连续
bool TextureArray::is_consecutive() {
  if (texture_array.empty()) {
    return true;
  } else {
    return texture_array.back()->texture_id + 1 == current_id_handle;
  }
}

// 完成纹理池构造
void TextureArray::finalize() {
  XINFO("完成处理纹理数组纹理池");
  // 创建纹理数组
  GLCALL(cvs->glGenTextures(1, &gl_texture_array_id));
  GLCALL(cvs->glBindTexture(GL_TEXTURE_2D_ARRAY, gl_texture_array_id));

  // 分配存储空间
  GLCALL(cvs->glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8,
                             texture_size.width(), texture_size.height(),
                             texture_array.size()));

  // 为每一层上传纹理数据
  for (int i = 0; i < texture_array.size(); i++) {
    GLCALL(cvs->glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i,
                                texture_size.width(), texture_size.height(), 1,
                                GL_RGBA, GL_UNSIGNED_BYTE,
                                texture_array[i]->texture_image.bits()));
    XINFO("[" + std::to_string(texture_array[i]->texture_id) +
          "]:" + texture_array[i]->name + "上传到gpu纹理数组");
  }

  // 设置纹理参数
  GLCALL(cvs->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER,
                              GL_NEAREST));
  GLCALL(cvs->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER,
                              GL_NEAREST));
  GLCALL(
      cvs->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT));
  GLCALL(
      cvs->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT));
}

// 使用纹理数组池
// 使用纹理集请在use后更新useatlas的uniform
void TextureArray::use(std::shared_ptr<BaseTexturePool> pool_reference,
                       std::shared_ptr<AbstractRenderer> renderer_context,
                       size_t batch_index) {
  bool need_update{false};
  // 检查渲染器当前正在使用的纹理池
  if (renderer_context->current_use_pool != pool_reference) {
    // 不是使用此纹理池
    need_update = true;
  }
  if (need_update) {
    // 更新设置渲染器当前正在使用的纹理池
    renderer_context->current_use_pool = pool_reference;
    // 更新用法uniform
    renderer_context->set_uniform_integer(
        "texture_pool_usage", static_cast<int>(TexturePoolType::ARRAY));
    // 更新samplerarray
    GLCALL(cvs->glActiveTexture(GL_TEXTURE15));
    GLCALL(cvs->glBindTexture(GL_TEXTURE_2D_ARRAY, gl_texture_array_id));
    renderer_context->set_sampler("samplerarray", 15);
    // 更新当前使用的采样器数组起始纹理id
    renderer_context->set_uniform_integer("arraystartoffset",
                                          first_texture_id_offset);
  }
}
