#ifndef TEXTURE_ARRAY_H
#define TEXTURE_ARRAY_H

#include <qsize.h>

#include <cstdint>
#include <memory>
#include <vector>

#include "BaseTexturePool.h"
#include "renderer/AbstractRenderer.h"

class TextureArray : public BaseTexturePool {
 public:
  // 构造TextureArray-opengl使用纹理数组必须相同分辨率
  TextureArray(GLCanvas* canvas, QSize size);
  // 析构TextureArray
  ~TextureArray() override;

  // 最大采样器层数
  static uint32_t max_texture_layer;

  // 固定相同的分辨率
  QSize texture_size;

  // 纹理数组管理
  std::vector<std::shared_ptr<TextureInstace>> texture_array;

  // 此纹理数组的openglid
  uint32_t gl_texture_array_id;

  // 首纹理id偏移
  uint32_t first_texture_id_offset;

  // 检查纹理id是否与当前纹理数组连续
  bool is_consecutive();

  // 判满
  bool is_full() override;

  // 载入纹理
  int load_texture(std::shared_ptr<TextureInstace> texture) override;

  // 完成纹理池构造
  void finalize() override;

  // 使用此纹理池
  // Base需使用指定批次
  // Array不需要
  void use(std::shared_ptr<BaseTexturePool> pool_reference,
           std::shared_ptr<AbstractRenderer> renderer_context,
           size_t batch_index = -1) override;

  // 取消使用此纹理池
  void unuse(std::shared_ptr<BaseTexturePool> pool_reference,
             std::shared_ptr<AbstractRenderer> renderer_context) override;
};

#endif  // TEXTURE_ARRAY_H
