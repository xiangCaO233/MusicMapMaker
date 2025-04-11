#ifndef M_FONT_H
#define M_FONT_H

#include <freetype/freetype.h>
#include <qpoint.h>
#include <qsize.h>

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

#include "../shader/Shader.h"
#include FT_FREETYPE_H

class GLCanvas;

// 字符位图
struct CharacterGlyph {
  // 纹理ID
  uint32_t glyph_id;
  // 字形尺寸
  QSize size;
  // 基线偏移
  QPoint bearing;
  // 水平步进
  uint32_t advance;
};

// 字符包
struct CharacterPack {
  // 这个字符包的像素大小
  uint32_t font_size;
  // 字符位图集(unicode)
  std::unordered_map<char32_t, CharacterGlyph> character_set;
};

class FontPool {
  // ftlibrary库
  static FT_Library ft;
  // ftlibrary库加载标识
  static bool is_ft_library_loaded;

  // 字体池数量
  static int pool_count;

  // 字体着色器
  std::shared_ptr<Shader> shader;

  // 字体Family名-按字体大小存储的字体字符包
  std::unordered_map<std::string, std::unordered_map<uint32_t, CharacterPack>>
      font_packs_mapping;

  // 字体gl对象
  uint32_t fVAO;
  uint32_t fVBO;

  // 实际采样使用的纹理数组(作为纹理集序列存储使用的字符位图)
  uint32_t glyphs_texture_array;

  // 每个字符位图纹理集的元数据---使用采样器数组存储
  uint32_t glyphs_atlas_meta_array;

  // 更新位图纹理的帧缓冲对象
  uint32_t fFBO;

  // gl上下文
  GLCanvas* cvs;

  // 字体Family名-FreeType称之为面(Face)的东西
  std::unordered_map<std::string, FT_Face> ft_faces;

  friend class RendererManager;

 public:
  // 构造Font
  FontPool(GLCanvas* canvas, std::shared_ptr<Shader> font_shader);

  // 析构Font
  virtual ~FontPool();

  // 绑定字体池
  void bind();

  // 取消绑定字体池
  void unbind();

  // 加载字体
  int load_font(const char* font_path);

  // 载入ascii字符
  void load_ascii(const std::string& font_name, FT_Face face);

  // 载入中文字符
  void load_cjk(const std::string& font_name, FT_Face face);
};

#endif  // M_FONT_H
