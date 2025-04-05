#ifndef BASE_TEXTURE_POOL_H
#define BASE_TEXTURE_POOL_H

#include <qimage.h>
#include <qsize.h>

#include <cstdint>
#include <string>
#include <unordered_map>

enum class TexturePoolType {
  BASE_POOL,
  ATLAS,
  ARRARY,
};

struct TextureInstace {
  // 名称
  std::string name;
  // 纹理id
  uint32_t texture_id;
  // 纹理实例
  std::unique_ptr<QImage> texture_image;

  // 纹理在图集中的位置
  uint32_t woffset{0};
  uint32_t hoffset{0};
};

class BaseTexturePool {
  friend class RendererManager;

 protected:
  // 纹理映射表(id-纹理对象)
  std::unordered_map<std::string, TextureInstace*> texture_map;
  // 纹理池类型
  TexturePoolType pool_type;

 public:
  static TextureInstace WHITE_PIXEL;
  // 构造TexturePool
  BaseTexturePool();
  // 析构TexturePool
  virtual ~BaseTexturePool();

  // 载入纹理
  virtual void load_texture(const char* resource_path);

  // 判满
  virtual bool is_full() = 0;

  // 使用此纹理池
  virtual void bind();
};

#endif  // BASE_TEXTURE_POOL_H
