#ifndef BASE_TEXTURE_POOL_H
#define BASE_TEXTURE_POOL_H

#include <memory>
#include <unordered_map>

#include "../Texture.h"
#define POOL_FULL 0x01
#define SUCCESS 0x01

class GLCanvas;

// 纹理池类型
enum class TexturePoolType {
  BASE_POOL,
  ARRARY,
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
  // 使用纹理数组时的采样器数组偏移地址
  uint32_t texture_array_offset;
};

class BaseTexturePool {
  friend class RendererManager;

  // gl窗口上下文
  GLCanvas* cvs;

 public:
  // 白像素
  static std::shared_ptr<TextureInstace> WHITE_PIXEL;
  // 纹理映射表(id-纹理对象)
  std::unordered_map<std::string, std::shared_ptr<TextureInstace>> texture_map;
  // 纹理池类型
  TexturePoolType pool_type;

  // 构造TexturePool
  BaseTexturePool(GLCanvas* canvas);
  // 析构TexturePool
  virtual ~BaseTexturePool();

  // 获取纹理指针
  std::shared_ptr<TextureInstace> get_texture(const std::string& name);

  // 载入纹理
  virtual int load_texture(std::shared_ptr<TextureInstace> texture) = 0;

  // 判满
  virtual bool is_full() = 0;

  // 使用此纹理池
  virtual void bind() = 0;

  // 取消使用此纹理池
  virtual void unbind() = 0;

  // 使用指定纹理
  // 使用单独纹理池时需设置连续的uniform
  virtual void use_texture(std::shared_ptr<TextureInstace> texture) = 0;
};

#endif  // BASE_TEXTURE_POOL_H
