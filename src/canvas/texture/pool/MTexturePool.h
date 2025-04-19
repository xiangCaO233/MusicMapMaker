#ifndef M_MTEXTUREPOOL_H
#define M_MTEXTUREPOOL_H

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "texture/pool/Atlas.h"

class GLCanvas;
class TextureInstace;
class AbstractRenderer;

class MTexturePool {
  // 驱动信息
  // 最大支持连续的采样器数
  static uint32_t max_sampler_consecutive_count;

  // 最大支持总采样器数
  static uint32_t max_total_sampler_count;

  // 最大采样器层数
  static uint32_t max_texture_layer;

  // cpu同步的显卡信息
  // 着色器使用了的纹理数组
  static std::vector<uint32_t> samplerarrays;

 public:
  // 构造MTexturePool
  MTexturePool(GLCanvas *canvas, uint32_t layer_width = 4096,
               uint32_t layer_height = 4096);

  // 析构MTexturePool
  virtual ~MTexturePool();

  // 使用的纹理单元
  int32_t sampler_unit{-1};

  // 纹理池相关
  // 全部纹理映射表(id-纹理对象)
  std::unordered_map<std::string, std::shared_ptr<TextureInstace>> texture_map;

  // 每层的尺寸
  uint32_t layerw, layerh;

  // 全部的纹理层
  std::vector<std::shared_ptr<Atlas>> layers;

  // 每层纹理集存储的纹理集数据
  uint32_t atlasMetaTextureArray;

  // 初始纹理最大层数
  size_t current_max_layer{16};

  // gl窗口上下文
  GLCanvas *cvs;

  // 需要更新采样器位置
  bool need_update_sampler_location{false};

  // 当前纹理数组id句柄
  uint32_t current_gl_texture_array_handle{0};

  // 扩展层数
  void expand_layer();

  // 初始化驱动信息
  static void init_driver_info(uint32_t texture_layer,
                               uint32_t total_sampler_count,
                               uint32_t sampler_consecutive_count);

  // 获取空闲纹理单元
  static size_t get_free_sampler_unit();

  // 判满
  bool isfull();

  // 添加纹理
  bool load_texture(std::shared_ptr<TextureInstace> &texture);

  // 使用纹理池
  void use(const std::shared_ptr<MTexturePool> &pool_reference,
           std::shared_ptr<AbstractRenderer> &renderer_context,
           size_t layer_index);
};

#endif  // M_MTEXTUREPOOL_H
