#ifndef TEXTURE_POOL_H
#define TEXTURE_POOL_H

#include <cstddef>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../Texture.h"
#include "BaseTexturePool.h"

class AbstractRenderer;

class TexturePool : public BaseTexturePool {
 public:
  // 构造TexturePool
  TexturePool(GLCanvas *canvas, bool dynamic_switch = true);

  // 析构TexturePool
  ~TexturePool() override;

  // 全部纹理批
  std::vector<std::vector<std::shared_ptr<TextureInstace>>> texture_dozens;

  // 纹理gl句柄映射表(纹理对象-gl句柄)
  std::unordered_map<std::shared_ptr<TextureInstace>, uint32_t> glhandler_map;

  // 纹理对应批次和批内索引的映射表(纹理-(批索引-批内索引))
  std::unordered_map<std::shared_ptr<TextureInstace>, std::pair<size_t, size_t>>
      batch_mapping;

  // 缓存的渲染器-批次表
  std::unordered_map<std::shared_ptr<AbstractRenderer>, size_t>
      temp_renderer_pool_mark;

  // 是否支持动态切换纹理(不支持则只初始化一组)
  bool dynamic_switch_texture;

  // 最大支持连续的采样器数
  static uint32_t max_sampler_consecutive_count;

  // 最大支持连续总采样器数
  static uint32_t max_total_sampler_count;

  // 判满
  bool is_full() override;

  // 载入纹理
  int load_texture(std::shared_ptr<TextureInstace> texture) override;

  // 完成纹理池构造
  void finalize() override;

  // 上传纹理集元数据组
  void upload_atlas_data() override;

  // 使用此纹理池
  // Base需使用指定批次
  // Array不需要
  void use(const std::shared_ptr<BaseTexturePool> &pool_reference,
           std::shared_ptr<AbstractRenderer> &renderer_context,
           size_t batch_index = -1) override;
};

#endif  // TEXTURE_POOL_H
