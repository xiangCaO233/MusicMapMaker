#include "FontPool.h"

#include <freetype/freetype.h>

#include <string>

#include "../../../log/colorful-log.h"
#include "../../GLCanvas.h"

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
FT_Library FontPool::ft;

// ftlibrary库加载标识
bool FontPool::is_ft_library_loaded = false;

// 字体池数量
int FontPool::pool_count = 0;

FontPool::FontPool(GLCanvas* canvas, std::shared_ptr<Shader> font_shader)
    : cvs(canvas), shader(font_shader) {
  pool_count++;
  // 检查字体库是否已初始化
  if (!is_ft_library_loaded) {
    // @return:
    //   FreeType error code.  0~means success.
    // FreeType函数在出现错误时将返回一个非零的整数值
    if (FT_Init_FreeType(&ft)) {
      XCRITICAL("FreeType初始化失败");
      return;
    } else {
      is_ft_library_loaded = true;
      XINFO("FreeType初始化成功");
    }
  }
  // 生成字体池的顶点数组对象
  GLCALL(canvas->glGenVertexArrays(1, &fVAO));
  // 生成字体池的顶点缓冲对象
  GLCALL(cvs->glGenBuffers(1, &fVBO));
}

FontPool::~FontPool() {
  pool_count--;
  if (pool_count == 0 && is_ft_library_loaded) {
    // 纹理池已全部释放
    FT_Done_FreeType(ft);
    // 恢复标识
    is_ft_library_loaded = false;
  }
}

// 绑定字体池
void FontPool::bind() {
  // 绑定顶点数组
  GLCALL(cvs->glBindVertexArray(fVAO));
  shader->use();
}

// 取消绑定字体池
void FontPool::unbind() {
  // 取消绑定顶点数组
  GLCALL(cvs->glBindVertexArray(0));
  shader->unuse();
}

// 加载字体
int FontPool::load_font(const char* font_path) {
  // 载入字体
  FT_Face face;
  FT_Error ret = FT_New_Face(ft, font_path, 0, &face);
  auto it = ft_faces.try_emplace(face->family_name, std::move(face)).first;

  if (ret != 0) {
    XERROR("加载字体" + std::string(font_path) + "失败");
  } else {
    XINFO("加载字体" + std::string(font_path) + "成功");

    // 载入常用字符
    load_ascii(it->first, it->second);
    load_cjk(it->first, it->second);
  }
  return ret;
}

// 载入ascii字符
void FontPool::load_ascii(const std::string& font_name, FT_Face face) {
  XINFO("正在生成ascii字符缓存");
}

// 载入中文字符
void FontPool::load_cjk(const std::string& font_name, FT_Face face) {
  XINFO("正在生成常用中文字符缓存");
}
