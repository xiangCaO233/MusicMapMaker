#ifndef M_FONTRENDERER_H
#define M_FONTRENDERER_H

#include <freetype/freetype.h>
#include <qpoint.h>
#include <qsize.h>

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

#include "../AbstractRenderer.h"
#include "../shader/Shader.h"
#include "renderer/font/GlyphPacker.h"
#include FT_FREETYPE_H

class GLCanvas;

// 字符位图
struct CharacterGlyph {
  // 纹理ID
  uint32_t glyph_id;
  // 在纹理集中的位置
  QPoint pos_in_atlas;
  // 字形尺寸
  // width	face->glyph->bitmap.width	位图宽度（像素）
  // height	face->glyph->bitmap.rows	位图高度（像素）
  QSize size;
  // 基线偏移
  // bearingX	face->glyph->bitmap_left
  // 水平距离，即位图相对于原点的水平位置（像素）
  // bearingY	face->glyph->bitmap_top
  // 垂直距离，即位图相对于基准线的垂直位置（像素）
  QPoint bearing;
  // 水平步进
  // 水平预留值，即原点到下一个字形原点的水平距离（单位：1/64像素）
  uint32_t xadvance;
};

// 字符包
struct CharacterPack {
  // 这个字符包的像素大小
  uint32_t font_size;

  // 字符位图集(unicode)
  std::unordered_map<char32_t, CharacterGlyph> character_set;
};

// 字符uv集数据
struct CharacterUVSet {
  QVector2D p1;
  QVector2D p2;
  QVector2D p3;
  QVector2D p4;
  bool operator==(const CharacterUVSet& other) const {
    return p1 == other.p1 && p2 == other.p2 && p3 == other.p3 && p4 == other.p4;
  };
};

class FontRenderer : public AbstractRenderer {
 public:
  // 构造FontRenderer
  FontRenderer(GLCanvas* canvas, std::shared_ptr<Shader> font_shader);
  // 析构FontRenderer
  ~FontRenderer() override;

  // 每层最大尺寸
  static const uint32_t layer_size{4096};

  // 最大层数
  static const uint32_t layer_count{32};

  // 当前字形id
  static uint32_t current_glyph_id;

  // ftlibrary库
  static FT_Library ft;

  // ftlibrary库加载标识
  static bool is_ft_library_loaded;

  // 全部字体渲染器数量
  static int frenderer_count;

  // 字体Family名-按字体大小存储的字体字符包
  std::unordered_map<std::string, std::unordered_map<uint32_t, CharacterPack>>
      font_packs_mapping;
  // 字体Family名-FreeType称之为面(Face)的东西
  std::unordered_map<const char*, FT_Face> ft_faces;

  // 层数-纹理集打包器
  std::unordered_map<uint32_t, std::shared_ptr<GlyphPacker>> layer_packers;
  // 当前最大的层索引
  uint32_t current_max_layer_index{0};
  // 空闲的层数
  std::vector<uint32_t> packable_layers;

  // 字体实例缓冲区
  uint32_t fInstanceVBO;
  // 实际采样使用的纹理数组(作为纹理集序列存储使用的字符位图)
  uint32_t glyphs_texture_array;
  // 预计更新gpu内存表(实例索引-实例数)
  std::vector<std::pair<size_t, uint32_t>> update_list;
  // 渲染器数据
  // 渲染器字符id集数据
  std::vector<float> glyph_id_data;
  // 渲染器字符uv集数据
  std::vector<CharacterUVSet> uvset_data;

  // 加载字体
  int load_font(const char* font_path);

  // 检查载入字符串
  void check_u8string(const std::u8string& str, uint32_t font_size,
                      FT_Face& face);

  // 同步数据
  void synchronize_data(InstanceDataType data_type, size_t instance_index,
                        void* data) override;

  // 同步更新位置标记
  void synchronize_update_mark(size_t instance_index);

  // 更新gpu数据
  void update_gpu_memory() override;

  // 重置更新内容
  void reset_update() override;
};

#endif  // M_FONTRENDERER_H
