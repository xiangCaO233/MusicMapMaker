#ifndef TEXTURE_ATLAS_H
#define TEXTURE_ATLAS_H

#include "../Texture.h"
#include "MaxRectsBinPack.h"

class TextureAtlas : public TextureInstace {
  // 打包器
  MaxRectsBinPack packer;

  // 是否完成打包
  bool is_packed{false};

  // 生成纹理集
  void pack();

  friend class RendererManager;

 public:
  // 构造TextureAtlas
  TextureAtlas(int width = 4096, int height = 4096, float expandrate = 1.05);

  // 析构TextureAtlas
  ~TextureAtlas();

  // 添加纹理
  void add_texture(const char* resource_path);
};

#endif  // TEXTURE_ATLAS_H
