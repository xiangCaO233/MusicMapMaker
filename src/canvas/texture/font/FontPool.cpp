#include "FontPool.h"

#include "../../../log/colorful-log.h"

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

// ftlibrary库
FT_Library* FontPool::ft = nullptr;
// 释放ftlibrary
void FontPool::free_library() {
  // 检查字体库是否已初始化
  if (ft) {
    if (FT_Done_FreeType(*ft)) {
      XCRITICAL("FreeType释放失败");
    } else {
      XINFO("FreeType释放成功");
    }
  } else {
    XERROR("FreeType未初始化");
  }
}

FontPool::FontPool(GLCanvas* canvas) : cvs(canvas) {
  // 检查字体库是否已初始化
  if (!ft) {
    // @return:
    //   FreeType error code.  0~means success.
    // FreeType函数在出现错误时将返回一个非零的整数值
    if (FT_Init_FreeType(ft)) {
      XCRITICAL("FreeType初始化失败");
      return;
    } else {
      XINFO("FreeType初始化成功");
    }
  }
}

FontPool::~FontPool() {}

// 加载字体
int FontPool::load_font(const char* font_path) {
  // 载入字体
  ft_faces.emplace_back();
  FT_Error ret;
  if (ret = FT_New_Face(*ft, font_path, 0, ft_faces.back())) {
    XERROR("加载字体" + std::string(font_path) + "失败");
  } else {
    XINFO("加载字体" + std::string(font_path) + "成功");
  }
  return ret;
}
