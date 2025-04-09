#ifndef BASE_TEXTURE_POOL_H
#define BASE_TEXTURE_POOL_H

#include <cstdint>
#include <memory>
#include <unordered_map>

#include "../Texture.h"

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

class BaseTexturePool {
 public:
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
};

#endif  // BASE_TEXTURE_POOL_H
