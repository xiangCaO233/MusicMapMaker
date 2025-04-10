#include "TextureAtlas.h"

#include <qpainter.h>

#include <cstdint>
#include <memory>
#include <string>

#include "../../../log/colorful-log.h"
#include "GLCanvas.h"
#include "texture/Texture.h"
#include "texture/atlas/MaxRectsBinPack.h"
#include "texture/pool/BaseTexturePool.h"

// 用于包装 OpenGL 调用并检查错误
#define GLCALL(func)                                       \
  func;                                                    \
  {                                                        \
    XLogger::glcalls++;                                    \
    GLenum error = cvs->glGetError();                      \
    if (error != GL_NO_ERROR) {                            \
      XERROR("在[" + std::string(#func) +                  \
             "]发生OpenGL错误: " + std::to_string(error)); \
    }                                                      \
  }

AtlasSubTexture::AtlasSubTexture(const char* path,
                                 std::shared_ptr<BaseTexturePool> preference)
    : TextureInstace(path, preference) {}
AtlasSubTexture::~AtlasSubTexture() = default;

// 全局纹理集序号
uint32_t TextureAtlas::global_atlas_index = 0;

TextureAtlas::TextureAtlas(GLCanvas* canvas,
                           std::shared_ptr<BaseTexturePool> pool_reference,
                           int width, int height, float expandrate)
    : TextureInstace(nullptr, pool_reference),
      cvs(canvas),
      packer(width, height, expandrate) {
  name = "atlas" + std::to_string(global_atlas_index);
  is_atlas = true;
  global_atlas_index++;
}

TextureAtlas::~TextureAtlas() = default;

// 添加纹理
std::shared_ptr<AtlasSubTexture> TextureAtlas::add_texture(
    const std::shared_ptr<BaseTexturePool>& pool_reference,
    const std::shared_ptr<TextureAtlas>& atlas_reference,
    const char* resource_path) {
  auto sub_texture =
      std::make_shared<AtlasSubTexture>(resource_path, pool_reference);

  // 赋值引用
  sub_texture->atlas_reference = atlas_reference;
  // 填入虚拟纹理集
  packer.Insert(sub_texture, MaxRectsBinPack::RectBottomLeftRule);

  // 创建映射
  sub_images.emplace_back(sub_texture);
  return sub_texture;
}

// 生成纹理集
void TextureAtlas::pack() {
  if (is_packed) {
    XERROR("Atlas is already packed");
  } else {
    // 完成打包-按矩形位置将这些纹理绘制到一个纹理对象上-qt软绘制
    // 填充并释放纹理数据
    texture_image =
        QImage(packer.binWidth, packer.binHeight, QImage::Format_RGBA8888);
    texture_image.fill(Qt::transparent);
    QPainter p(&texture_image);

    for (const auto& sub_texture : sub_images) {
      // 放置纹理到atlas
      // 绘制子图像到主纹理的指定位置
      p.drawImage(QPoint(sub_texture->woffset, sub_texture->hoffset),
                  sub_texture->texture_image);
    }
    XINFO("最终纹理集大小:" + std::to_string(packer.binWidth) + "x" +
          std::to_string(packer.binHeight) + "]");
    XINFO("纹理集填充率:[" + std::to_string(packer.Occupancy()) + "]");
    // 生成纹理元数据
    // atlas_meta_data[0] = int unit_index;
    // 需在使用纹理池finalize函数内更新
    // // 使用单独-图集方式时
    // 映射实际采样的纹理单元
    // 使用采样器数组-图集方式时
    // 映射的实际采样的纹理数组层级
    // 子纹理数量
    // float sub_count;
    atlas_meta_data[1] = sub_images.size();
    // 图集的总尺寸
    // vec2 size;
    atlas_meta_data[2] = packer.binWidth;
    atlas_meta_data[3] = packer.binHeight;

    // 子纹理元数据集
    // AtlasSubMeta sub_metas[512];
    for (int i{0}; i < sub_images.size(); i++) {
      // 图集子纹理元数据结构
      // struct AtlasSubMeta {
      //   // 使用的纹理在图集中的x位置
      //   vec2 position;
      //   // 使用的纹理的尺寸
      //   vec2 size;
      // };
      // 以上16b,4f
      atlas_meta_data[4 + i] = sub_images[i]->woffset;
      atlas_meta_data[4 + i + 1] = sub_images[i]->hoffset;
      atlas_meta_data[4 + i + 2] = sub_images[i]->texture_image.width();
      atlas_meta_data[4 + i + 3] = sub_images[i]->texture_image.height();
    }

    is_packed = true;
  }
}

// 生成子纹理id
void TextureAtlas::generate_subid() {
  if (is_subimage_id_generated) return;
  // 只生成一次
  // 编码子纹理id
  // 8位(bit 17~24)表示纹理集编号
  uint8_t texture_set = atlas_meta_data[0];
  for (int i{0}; i < sub_images.size(); i++) {
    // 低16位(bit 0-16)表示子纹理编号
    uint32_t sub_textureindex = i;
    sub_images[i]->texture_id =
        ((uint32_t)texture_set << 16) | sub_textureindex;
  }
}

// 获取尺寸
QSize TextureAtlas::size() { return {packer.binWidth, packer.binWidth}; }

// 纹理集是否已满
bool TextureAtlas::is_full() {
  return sub_images.size() >= max_subimage_count ||
         packer.binWidth >= def_max_size.width() &&
             packer.binHeight >= def_max_size.height();
}
