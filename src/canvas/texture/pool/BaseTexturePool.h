#ifndef BASE_TEXTURE_POOL_H
#define BASE_TEXTURE_POOL_H

#include <cstdint>
#include <memory>
#include <unordered_map>

#include "../Texture.h"
#include "renderer/AbstractRenderer.h"

#define POOL_FULL 0x01
#define EXPECTED_SIZE 0x02
#define NOT_CONSECUTIVE 0x03
#define SUCCESS 0x04

class GLCanvas;
class AbstractRenderer;
class BaseTexturePool;

// 纹理池类型
enum class TexturePoolType : int32_t {
  BASE_POOL = 1,
  ARRAY = 2,
};

// 纹理uniform(传递到shader)
struct TextureUniformData {
  // 标记使用方式
  // 0-单独使用采样器
  // 1-使用纹理图集
  // 2-使用同尺寸纹理采样器数组
  uint32_t texture_pool_usage;
  // 要使用的采样器上下文数组
  // (长度由显卡驱动实际支持长度为准)
  // static uint32_t max_sampler_consecutive_count;
  // 使用方式为单独或纹理图集时需要
  uint32_t* sampler_contexts;
};

class BaseTexturePool {
 public:
  // 白像素
  static std::shared_ptr<TextureInstace> WHITE_PIXEL;

  // gl窗口上下文
  GLCanvas* cvs;

  // 纹理映射表(id-纹理对象)
  std::unordered_map<std::string, std::shared_ptr<TextureInstace>> texture_map;

  // 纹理池类型
  TexturePoolType pool_type;

  // 当前纹理id句柄
  uint32_t current_id_handle{0};

  // 是否完成导入
  bool is_finalized;

  // 构造TexturePool
  BaseTexturePool(GLCanvas* canvas);
  // 析构TexturePool
  virtual ~BaseTexturePool();

  // 获取纹理指针
  std::shared_ptr<TextureInstace> get_texture(const std::string& name);

  // 载入纹理
  virtual int load_texture(std::shared_ptr<TextureInstace> texture) = 0;

  // 完成纹理池构造
  virtual void finalize() = 0;

  // 判满
  virtual bool is_full() = 0;

  // 使用此纹理池
  // Base需使用指定批次
  // Array不需要
  virtual void use(std::shared_ptr<BaseTexturePool> pool_reference,
                   std::shared_ptr<AbstractRenderer> renderer_context,
                   size_t batch_index = -1) = 0;

  // 取消使用此纹理池
  virtual void unuse(std::shared_ptr<BaseTexturePool> pool_reference,
                     std::shared_ptr<AbstractRenderer> renderer_context) = 0;
};

#endif  // BASE_TEXTURE_POOL_H
