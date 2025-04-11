#ifndef TEXTURE_ATLAS_H
#define TEXTURE_ATLAS_H

#include <cstdint>
#include <memory>

#include "../Texture.h"
#include "GLCanvas.h"
#include "MaxRectsBinPack.h"
/*
图集子纹理元数据
struct AtlasSubMeta {
  使用的纹理在图集中的x位置
  vec2 position;
  使用的纹理的尺寸
  vec2 size;
};

图集元数据
struct AtlasMeta {
  使用单独-图集方式时
  映射实际采样的纹理单元
  使用采样器数组-图集方式时
  映射的实际采样的纹理数组层级--索引:0
  int unit_index;
  子纹理数量--索引:1
  float sub_count;
  图集的总尺寸--索引:2,3
  vec2 size;
  子纹理元数据集--索引头:4
  AtlasSubMeta sub_metas[512];
};
*/
/*
每个AtlasMeta存储在纹理数组的一个layer中
每个layer尺寸为足够存储2052个float的2D纹理
使用RGBA32F格式(每个纹素4个float)
纹理尺寸:
2052 floats / 4 = 513纹素 →
32×16=512 + 1 →
使用32×17=544纹素
*/

class TextureAtlas;
class GLCanvas;

// 成为子纹理后不可直接使用(需配合对应的纹理集使用)
// texture_id意义发生变化
class AtlasSubTexture : public TextureInstace {
 public:
  AtlasSubTexture(const char* path,
                  std::shared_ptr<BaseTexturePool> preference);
  ~AtlasSubTexture() override;
  std::shared_ptr<TextureAtlas> atlas_reference;
  // 纹理在图集中的位置
  uint32_t woffset{0};
  uint32_t hoffset{0};
};

class TextureAtlas : public TextureInstace {
 public:
  // 构造TextureAtlas
  TextureAtlas(GLCanvas* canvas,
               std::shared_ptr<BaseTexturePool> pool_reference,
               int width = 4096, int height = 4096, float expandrate = 1.05);

  // 析构TextureAtlas
  ~TextureAtlas();

  // 全局纹理集序号
  static uint32_t global_atlas_index;

  // 纹理集特有的裸指针引用
  BaseTexturePool* preference;

  // gl上下文引用
  GLCanvas* cvs;

  // 打包器
  MaxRectsBinPack packer;

  // 纹理集的gl句柄
  uint32_t atlas_gl_handle;

  // 是否完成打包
  bool is_packed{false};

  // 是否生成子纹理id
  bool is_subimage_id_generated{false};

  // 默认最大图集尺寸
  QSize def_max_size{8345, 8345};

  // 默认最大图集子纹理数量
  const uint32_t max_subimage_count{512};

  // 纹理元数据存储空间
  float atlas_meta_data[2052];

  // 纹理集内部全部子纹理映射表(id-纹理)
  std::vector<std::shared_ptr<AtlasSubTexture>> sub_images;

  // 添加纹理
  std::shared_ptr<AtlasSubTexture> add_texture(
      const std::shared_ptr<BaseTexturePool>& pool_reference,
      const std::shared_ptr<TextureAtlas>& atlas_reference,
      const char* resource_path);

  // 生成纹理集
  void pack();

  // 生成子纹理id
  void generate_subid();

  // 纹理集是否已满
  bool is_full();

  // 获取尺寸
  QSize size();
};

#endif  // TEXTURE_ATLAS_H
