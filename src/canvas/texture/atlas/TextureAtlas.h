#ifndef TEXTURE_ATLAS_H
#define TEXTURE_ATLAS_H

#include "../Texture.h"
#include "MaxRectsBinPack.h"

class AtlasSubTexture : public TextureInstace {
 public:
  // 纹理在图集中的位置
  uint32_t woffset{0};
  uint32_t hoffset{0};
};

class TextureAtlas : public TextureInstace {
  // 打包器
  MaxRectsBinPack packer;

  // 是否完成打包
  bool is_packed{false};

  // 生成纹理集
  void pack();

  // 默认最大图集尺寸
  QSize def_max_size{8192, 8192};

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
