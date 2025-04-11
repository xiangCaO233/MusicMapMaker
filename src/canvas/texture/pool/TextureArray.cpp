#include "TextureArray.h"

#include <qtypes.h>

#include <cstdint>
#include <memory>

#include "../../../log/colorful-log.h"
#include "../../GLCanvas.h"
#include "texture/atlas/TextureAtlas.h"

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

TextureArray::TextureArray(GLCanvas* canvas, QSize size, bool use_atlas)
    : BaseTexturePool(canvas) {
  XINFO("构造纹理数组纹理池");
  pool_type = TexturePoolType::ARRAY;
  this->use_atlas = use_atlas;
  if (!use_atlas) {
    // 不是创建纹理集池时先初始化通用尺寸
    texture_size = size;
  }
}

TextureArray::~TextureArray() {}

// 判满
bool TextureArray::is_full() { return texture_map.size() >= max_texture_layer; }

// 载入纹理
int TextureArray::load_texture(std::shared_ptr<TextureInstace> texture) {
  // 检查载入的是否是纹理集
  auto texture_atlas = std::dynamic_pointer_cast<TextureAtlas>(texture);
  if (texture_atlas) {
    // 载入纹理集
    if (texture_map.empty()) {
      // 第一次载入纹理集
      use_atlas = true;
    } else {
      if (use_atlas = false) {
        // 并非纹理集纹理池
        return POOL_TYPE_NOTADAPT;
      }
    }
    texture_atlas->preference = this;
  } else {
    if (texture_map.empty()) {
      // 第一次载入非纹理集
      use_atlas = false;
    }
  }
  if (is_full()) return POOL_FULL;
  // 添加前检查
  bool expected_size{false};
  if (use_atlas) {
    if (texture_map.empty()) {
      // 使用首次添加的纹理集尺寸作为纹理采样数组池的通用尺寸
      texture_size = texture_atlas->size();
    }
    if (texture_size != QSizeF(texture_atlas->packer.binWidth,
                               texture_atlas->packer.binHeight)) {
      expected_size = true;
    }
  }
  if (texture->texture_image.size() != texture_size) expected_size = true;
  if (expected_size) return EXPECTED_SIZE;

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
  if (is_finalized) return;
  XINFO("完成处理纹理数组纹理池");
  // 创建纹理数组
  GLCALL(cvs->glGenTextures(1, &gl_texture_array_id));
  // 激活纹理单元
  GLCALL(cvs->glActiveTexture(GL_TEXTURE0 + 15));
  GLCALL(cvs->glBindTexture(GL_TEXTURE_2D_ARRAY, gl_texture_array_id));

  // 分配存储空间
  GLCALL(cvs->glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8,
                             texture_size.width(), texture_size.height(),
                             texture_array.size()));

  // 为每一层上传纹理数据
  for (int i = 0; i < texture_array.size(); i++) {
    auto& texture = texture_array[i];
    GLCALL(cvs->glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i,
                                texture_size.width(), texture_size.height(), 1,
                                GL_RGBA, GL_UNSIGNED_BYTE,
                                texture->texture_image.bits()));
    XINFO("[" + std::to_string(texture->texture_id) + "]:" + texture->name +
          "上传到gpu纹理数组");
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
  is_finalized = true;

  if (use_atlas) {
    XWARN("当前纹理池使用纹理集");
    // 初始化纹理数组存储的纹理集数据组
    upload_atlas_data();
  }
}

// 上传纹理集元数据组
void TextureArray::upload_atlas_data() {
  BaseTexturePool::upload_atlas_data();

  // 创建32宽×17高×N层的RGBA32F纹理数组
  GLCALL(cvs->glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA32F, 32, 17,
                           texture_array.size(), 0, GL_RGBA, GL_FLOAT,
                           nullptr));

  for (int i = 0; i < texture_array.size(); i++) {
    // atlas_meta_data[0] = int unit_index;
    // 需在使用纹理池finalize函数内更新
    auto texture_atlas =
        std::dynamic_pointer_cast<TextureAtlas>(texture_array[i]);
    texture_atlas->atlas_meta_data[0] = i;
    // 生成子纹理id
    texture_atlas->generate_subid();
    // 上传到对应层
    GLCALL(cvs->glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, 32, 17, 1, GL_RGBA,
                           GL_FLOAT, texture_atlas->atlas_meta_data));
    XINFO("metadata:");
    XINFO("unit_index:" + std::to_string(texture_atlas->atlas_meta_data[0]));
    XINFO("sub_count:" + std::to_string(texture_atlas->atlas_meta_data[1]));
    XINFO("atlas_size:[" + std::to_string(texture_atlas->atlas_meta_data[2]) +
          "," + std::to_string(texture_atlas->atlas_meta_data[3]) + "]");
  }
}

// 使用纹理数组池
// 使用纹理集请在use后更新useatlas的uniform
void TextureArray::use(const std::shared_ptr<BaseTexturePool>& pool_reference,
                       std::shared_ptr<AbstractRenderer>& renderer_context,
                       size_t batch_index) {
  // 检查更新纹理池使用类型
  if (renderer_context->current_pool_type != pool_type) {
    renderer_context->set_uniform_integer("texture_pool_usage",
                                          static_cast<int>(pool_type));
    renderer_context->current_pool_type = pool_type;
  }
  // 检查更新是否使用纹理集
  if (renderer_context->is_current_atlas != use_atlas) {
    renderer_context->set_uniform_integer("useatlas", use_atlas ? 1 : 0);
    renderer_context->is_current_atlas = use_atlas;
  }

  bool need_update{false};
  // 检查渲染器当前正在使用的纹理池
  if (renderer_context->current_use_pool != pool_reference) {
    // 不是使用此纹理池
    need_update = true;
  }

  // glViewPort后uniform的采样器location会发生变化,需要更新
  if (GLCanvas::need_update_sampler_location) {
    need_update = true;
    GLCanvas::need_update_sampler_location = false;
  }

  if (need_update) {
    // 更新设置渲染器当前正在使用的纹理池
    renderer_context->current_use_pool = pool_reference;
    // 更新samplerarray
    GLCALL(cvs->glActiveTexture(GL_TEXTURE15));
    GLCALL(cvs->glBindTexture(GL_TEXTURE_2D_ARRAY, gl_texture_array_id));
    renderer_context->set_sampler("samplerarray", 15);
    // 更新当前使用的采样器数组起始纹理id
    renderer_context->set_uniform_integer("arraystartoffset",
                                          first_texture_id_offset);
    if (use_atlas) {
      // 绑定纹理数组存储的纹理集数据组到纹理单元14
      renderer_context->set_sampler("atlas_meta_buffer_array", 14);
    }
  }
}
