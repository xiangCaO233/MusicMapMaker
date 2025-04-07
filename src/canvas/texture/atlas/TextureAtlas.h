#ifndef TEXTURE_ATLAS_H
#define TEXTURE_ATLAS_H

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

#include "../Texture.h"
#include "MaxRectsBinPack.h"

/*
 *纹理元数据
 *struct TextureMeta {
 *float woffset;
 *float hoffset;
 *float width;
 *float height;
 *
 *};
 * UBO,大小固定为最大纹理集子纹理数
 *layout(std140)
 *uniform TextureMetaBuffer {
 *float atlas_width;
 *float atlas_height;
 *int sub_image_count;
 *TextureMeta textureMetas[1024];
 *
 *};
 */

struct TextureMeta {
  float woffset;
  float hoffset;
  float width;
  float height;
};

struct TextureMetaBuffer {
  float atlas_width;
  float atlas_height;
  int sub_image_count;
  std::vector<TextureMeta> metas;
};

class TextureAtlas;

// 成为子纹理后不可直接使用(需配合对应的纹理集使用)
// texture_id意义发生变化
class AtlasSubTexture : public TextureInstace {
 public:
  std::shared_ptr<TextureAtlas> atlas_reference;
  // 纹理在图集中的位置
  uint32_t woffset{0};
  uint32_t hoffset{0};
};

class TextureAtlas : public TextureInstace {
 public:
  // 构造TextureAtlas
  TextureAtlas(int width = 4096, int height = 4096, float expandrate = 1.05);

  // 析构TextureAtlas
  ~TextureAtlas();

  // 打包器
  MaxRectsBinPack packer;

  // 子纹理id
  uint32_t sub_texture_id_handle{0};

  // 是否完成打包
  bool is_packed{false};

  // 默认最大图集尺寸
  QSize def_max_size{8345, 8345};

  // 默认最大图集子纹理数量
  const uint32_t max_subimage_count{1024};

  // 全部子纹理映射表(id-纹理)
  std::unordered_map<uint32_t, std::shared_ptr<AtlasSubTexture>> sub_images;

  // 子纹理gpu数据块
  TextureMetaBuffer buffer;

  // 添加纹理
  void add_texture(const char* resource_path);

  // 生成纹理集
  void pack();

  // 纹理集是否已满
  bool is_full();

  // 获取尺寸
  QSize size();

  // gpu buffer
  TextureMetaBuffer& gpu_buffer_data();
};

#endif  // TEXTURE_ATLAS_H
