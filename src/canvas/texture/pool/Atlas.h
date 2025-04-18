#ifndef M_ATLAS_H
#define M_ATLAS_H

#include <memory>
#include <vector>

#include "MaxRectsBinPack.h"

class Atlas {
 public:
  // 构造Atlas
  Atlas(uint32_t width, uint32_t height);

  // 析构Atlas
  virtual ~Atlas();

  // 默认最大图集子纹理数量
  const uint32_t max_subimage_count{512};

  // 打包器
  MaxRectsBinPack packer;

  // 纹理集内的全部子纹理
  std::vector<std::shared_ptr<TextureInstace>> sub_textures;

  // 是否已满
  bool isfull();

  // 向纹理集添加纹理
  bool add_texture(std::shared_ptr<TextureInstace> &texture);
};

#endif  // M_ATLAS_H
