#ifndef M_FONT_H
#define M_FONT_H

#include <freetype2/ft2build.h>

#include <vector>
#include FT_FREETYPE_H

class GLCanvas;

class FontPool {
  // ftlibrary库
  static FT_Library* ft;

  // gl上下文
  GLCanvas* cvs;

  // FreeType称之为面(Face)的东西
  std::vector<FT_Face*> ft_faces;

 public:
  // 构造Font
  FontPool(GLCanvas* canvas);
  // 析构Font
  virtual ~FontPool();

  // 释放ftlibrary
  static void free_library();

  // 加载字体
  int load_font(const char* font_path);
};

#endif  // M_FONT_H
