#include "GLCanvas.h"

#include <GL/gl.h>
#include <qcolor.h>
#include <qdir.h>
#include <qdiriterator.h>
#include <qlogging.h>
#include <qobject.h>
#include <qpaintdevice.h>
#include <qpainter.h>
#include <qpoint.h>
#include <qtypes.h>

#include <QFile>
#include <QMouseEvent>
#include <QRandomGenerator>
#include <QTextStream>
#include <QTimer>
#include <chrono>
#include <memory>
#include <string>

#include "../util/mutil.h"
#include "colorful-log.h"
#include "renderer/font/FontRenderer.h"
#include "texture/pool/MTexturePool.h"

// 用于包装 OpenGL 调用并检查错误
#define GLCALL(func)                                       \
  func;                                                    \
  {                                                        \
    XLogger::glcalls++;                                    \
    GLenum error = glGetError();                           \
    if (error != GL_NO_ERROR) {                            \
      XERROR("在[" + std::string(#func) +                  \
             "]发生OpenGL错误: " + std::to_string(error)); \
    }                                                      \
  }

bool GLCanvas::need_update_sampler_location = false;

GLCanvas::GLCanvas(QWidget *parent) : QOpenGLWidget(parent) {
  // 启用鼠标跟踪
  setMouseTracking(true);
  // setUpdateBehavior(QOpenGLWidget::PartialUpdate);
}

GLCanvas::~GLCanvas() {
  // 释放渲染管理器
  if (renderer_manager) delete renderer_manager;
};

// qt事件
// 鼠标按下事件
void GLCanvas::mousePressEvent(QMouseEvent *event) {
  // 传递事件
  QOpenGLWidget::mousePressEvent(event);
}

// 鼠标释放事件
void GLCanvas::mouseReleaseEvent(QMouseEvent *event) {
  // 传递事件
  QOpenGLWidget::mouseReleaseEvent(event);
}

// 鼠标双击事件
void GLCanvas::mouseDoubleClickEvent(QMouseEvent *event) {
  // 传递事件
  QOpenGLWidget::mouseDoubleClickEvent(event);
}

// 鼠标移动事件
void GLCanvas::mouseMoveEvent(QMouseEvent *event) {
  // 传递事件
  QOpenGLWidget::mouseMoveEvent(event);
  mouse_pos = event->pos();
  repaint();
}

// 鼠标滚动事件
void GLCanvas::wheelEvent(QWheelEvent *event) {
  // 传递事件
  QOpenGLWidget::wheelEvent(event);
}

// 键盘按下事件
void GLCanvas::keyPressEvent(QKeyEvent *event) {
  // 传递事件
  QOpenGLWidget::keyPressEvent(event);
}

// 键盘释放事件
void GLCanvas::keyReleaseEvent(QKeyEvent *event) {
  // 传递事件
  QOpenGLWidget::keyReleaseEvent(event);
}

// 取得焦点事件
void GLCanvas::focusInEvent(QFocusEvent *event) {
  // 传递事件
  QOpenGLWidget::focusInEvent(event);
}

// 失去焦点事件
void GLCanvas::focusOutEvent(QFocusEvent *event) {
  // 传递事件
  QOpenGLWidget::focusOutEvent(event);
}

// 进入事件
void GLCanvas::enterEvent(QEnterEvent *event) {
  // 传递事件
  QOpenGLWidget::enterEvent(event);
}

// 退出事件
void GLCanvas::leaveEvent(QEvent *event) {
  // 传递事件
  QOpenGLWidget::leaveEvent(event);
}

// 调整尺寸事件
void GLCanvas::resizeEvent(QResizeEvent *event) {
  // 传递事件
  QOpenGLWidget::resizeEvent(event);
}

void GLCanvas::initializeGL() {
  XINFO("初始化OpenGL函数");
  initializeOpenGLFunctions();
  // 查询opengl版本
  auto version = GLCALL(glGetString(GL_VERSION));
  XINFO("OpenGL 版本: " + std::string(reinterpret_cast<const char *>(version)));

  // 查询最大支持多层纹理的最大层数
  GLint maxLayers;
  glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &maxLayers);
  XINFO("多层纹理最大层数: " + std::to_string(maxLayers));

  // 查询纹理采样器最大连续数量
  GLint max_fragment_samplers;
  glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_fragment_samplers);
  XINFO("纹理采样器最大连续数量: " + std::to_string(max_fragment_samplers / 2));
  if (max_fragment_samplers > 16) {
    max_fragment_samplers = 16;
  }

  // 查询纹理采样器最大数量
  GLint max_combined_samplers;
  glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_combined_samplers);
  XINFO("纹理采样器最大数量: " + std::to_string(max_combined_samplers));

  // 查询最大支持抗锯齿MSAA倍率
  GLint maxSamples;
  GLCALL(glGetIntegerv(GL_MAX_SAMPLES, &maxSamples));

  // 初始化纹理池驱动信息
  MTexturePool::init_driver_info(maxLayers, max_combined_samplers,
                                 max_fragment_samplers);

  XINFO("启用最大抗锯齿倍率: " + std::to_string(maxSamples));
  // 启用 最大 MSAA
  context()->format().setSamples(maxSamples);

  XINFO("启用垂直同步");
  // 启用V-Sync
  context()->format().setSwapInterval(0);

  // 检查最大ubo size
  int maxUBOSize;
  GLCALL(glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUBOSize));
  XINFO("最大UBO块容量: " + std::to_string(maxUBOSize));

  // 标准混合模式
  GLCALL(glEnable(GL_BLEND));
  GLCALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

  // 初始化渲染管理器
  renderer_manager = new RendererManager(this, 64, 4096);

  // 初始化默认纹理
  load_texture_from_path("../resources/textures/default");

  // 初始化纹理
  // load_texture_from_path("../resources/textures/test/other");
  load_texture_from_path("../resources/textures/test/1024");

  add_texture("../resources/map/Designant - Designant/bg.jpg");
}
void GLCanvas::resizeGL(int w, int h) {
  GLCALL(glViewport(0, 0, w, h));
  // 投影矩阵
  QMatrix4x4 proj;
  // 计算正交投影矩阵
  proj.ortho(-(float)w / 2.0f, (float)w / 2.0f, -(float)h / 2.0f,
             (float)h / 2.0f, -1.0f, 1.0f);
  // 反转y轴
  proj.scale(1.0f, -1.0f, 1.0f);

  // 更新uniform
  renderer_manager->update_all_projection_mat("projection_mat", proj);

  // 标记需要更新纹理采样器位置
  renderer_manager->font_renderer->need_update_sampler_location = true;
  for (const auto &pool : renderer_manager->texturepools) {
    pool->need_update_sampler_location = true;
  }
}

// 绘制画布
void GLCanvas::paintGL() {
  static long pre_frame_time = 100;
  XLogger::glcalls = 0;
  XLogger::drawcalls = 0;
  auto before = std::chrono::high_resolution_clock::now().time_since_epoch();
  // 背景色
  GLCALL(glClearColor(0.23f, 0.23f, 0.23f, 1.0f));
  GLCALL(glClear(GL_COLOR_BUFFER_BIT));

  // 执行渲染
  // push_shape();

  // DEBUG
  // renderer_manager->addRect(QRectF(30, 30, 160, 90),
  // texture_full_map["bg.jpg"],
  //                           QColor(0, 0, 0, 255), 0, false);
  // renderer_manager->addRect(QRectF(100, 100, 128, 30),
  //                           texture_full_map["White.png"], QColor(0, 0, 0,
  //                           255), 0, true);

  auto theoretical_fps = std::to_string(
      int(std::round(1.0 / (((float)pre_update_frame_time) / 1000000.0))));
  auto glcall = std::to_string(pre_glcalls);
  auto drawcall = std::to_string(pre_drawcall);
// 创建 UTF-8 到 UTF-32 的转换器
#ifdef _WIN32
  // 使用 Windows API 转换
  std::u32string fpsstr = mutil::utf8_to_utf32(theoretical_fps);
  std::u32string glcallstr = mutil::utf8_to_utf32(glcall);
  std::u32string drawcallstr = mutil::utf8_to_utf32(drawcall);
#else
  std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
  std::u32string fpsstr = converter.from_bytes(theoretical_fps);
  std::u32string glcallstr = converter.from_bytes(glcall);
  std::u32string drawcallstr = converter.from_bytes(drawcall);
#endif  //_WIN32
  if (std::chrono::duration_cast<std::chrono::milliseconds>(before).count() -
          pre_update_fps >
      200) {
    pre_update_frame_time = pre_frame_time;
    pre_update_fps =
        std::chrono::duration_cast<std::chrono::milliseconds>(before).count();
  }
  fpsstr = fpsstr + U"fps";
  glcallstr = U"glcalls:" + glcallstr;
  drawcallstr = U"drawcalls:" + drawcallstr;
  // renderer_manager->addText(QPointF(0, 30), fpsstr, 24,
  //                           "ComicShannsMono Nerd Font", QColor(255, 183,
  //                           197), 0.0f);
  // renderer_manager->addText(QPointF(0, 56), glcallstr, 24,
  //                           "ComicShannsMono Nerd Font", QColor(255, 183,
  //                           197), 0.0f);
  // renderer_manager->addText(QPointF(0, 82), drawcallstr, 24,
  //                           "ComicShannsMono Nerd Font", QColor(255, 183,
  //                           197), 0.0f);

  QRectF rec(100, 100, 200, 200);

  renderer_manager->addRect(rec, texture_full_map["yuanchou.png"],
                            QColor(255, 255, 255), 0.0f, true);

  renderer_manager->addLine(QPointF(0, 0), QPointF(200, 200), 20.0f, nullptr,
                            QColor(0, 0, 0, 255), true);

  renderer_manager->renderAll();

  auto after = std::chrono::high_resolution_clock::now().time_since_epoch();
  pre_frame_time =
      std::chrono::duration_cast<std::chrono::microseconds>(after - before)
          .count();

  pre_glcalls = XLogger::glcalls;
  pre_drawcall = XLogger::drawcalls;
}

// 渲染实际图形
void GLCanvas::push_shape() {}

// 从指定目录添加纹理
void GLCanvas::load_texture_from_path(const char *p) {
  QString qps(p);
  QString path = QDir::currentPath() + "/" + qps + "/";
  QDir dir(path);
  if (!dir.exists()) {
    qWarning() << "Directory does not exist:" << path;
    return;
  }

  // 筛选图片文件
  static const std::unordered_set<std::string> image_extention = {"png", "jpg",
                                                                  "jpeg"};
  // 递归遍历所有文件和子目录
  QDirIterator it(path, QDirIterator::Subdirectories);

  while (it.hasNext()) {
    QString filePath = it.next();
    auto finfo = it.fileInfo();
    auto extention = finfo.suffix().toStdString();
    if (finfo.isFile()) {
      if (image_extention.find(extention) != image_extention.end()) {
        std::string filestr = filePath.toStdString();
        auto file = filestr.c_str();
        add_texture(file);
      }
    }
  }
}
// 设置垂直同步
void GLCanvas::set_Vsync(bool flag) {
  // 切换V-Sync
  context()->format().setSwapInterval(flag);
}

// 添加纹理
void GLCanvas::add_texture(const char *qrc_path) {
  // 是否需要新建纹理池
  bool need_new_pool{true};
  std::shared_ptr<MTexturePool> pool;
  for (const auto &p : renderer_manager->texturepools) {
    if (!p->isfull()) {
      // 有未满的纹理池,就用这个
      need_new_pool = false;
      pool = p;
    }
  }

  // 需要新建时
  if (need_new_pool) {
    pool = std::make_shared<MTexturePool>(this);
    // 加入纹理池表
    renderer_manager->texturepools.emplace_back(pool);
  }

  // 初始化纹理
  auto texture = std::make_shared<TextureInstace>(qrc_path, pool);

  // 添加到纹理池
  auto res = pool->load_texture(texture);

  if (res) {
    // 成功添加映射
    texture_full_map.try_emplace(texture->name, texture);
  }
}
