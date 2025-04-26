#include "GLCanvas.h"

#include <GL/gl.h>
#include <qcolor.h>
#include <qdir.h>
#include <qdiriterator.h>
#include <qlogging.h>
#include <qobject.h>
#include <qopenglversionfunctions.h>
#include <qpaintdevice.h>
#include <qpainter.h>
#include <qpoint.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include <QFile>
#include <QMouseEvent>
#include <QRandomGenerator>
#include <QTextStream>
#include <QThread>
#include <QTimer>
#include <chrono>
#include <filesystem>
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

GLCanvas::GLCanvas(QWidget *parent) {
  // 初始化帧率计数器
  fpsCounter = new FrameRateCounter();

  // 更新fps显示内容
  connect(fpsCounter, &FrameRateCounter::fpsUpdated, this,
          &GLCanvas::updateFpsDisplay);

  // 获取主屏幕的刷新率
  QScreen *primaryScreen = QGuiApplication::primaryScreen();
  float refreshRate = primaryScreen->refreshRate();

  XINFO("显示器刷新率:" + std::to_string(refreshRate) + "Hz");

  // 垂直同步帧间隔
  des_update_time = 1000.0 / refreshRate;
}

GLCanvas::~GLCanvas() {
  // 释放渲染管理器
  if (renderer_manager) delete renderer_manager;
  // 确保刷新线程退出
  stop_refresh = true;
  // update_thread.join();
};

// 更新fps显示
void GLCanvas::updateFpsDisplay(int fps) {}

// qt事件
// 鼠标按下事件
void GLCanvas::mousePressEvent(QMouseEvent *event) {
  // 传递事件
  QOpenGLWindow::mousePressEvent(event);
}

// 鼠标释放事件
void GLCanvas::mouseReleaseEvent(QMouseEvent *event) {
  // 传递事件
  QOpenGLWindow::mouseReleaseEvent(event);
}

// 鼠标双击事件
void GLCanvas::mouseDoubleClickEvent(QMouseEvent *event) {
  // 传递事件
  QOpenGLWindow::mouseDoubleClickEvent(event);
}

// 鼠标移动事件
void GLCanvas::mouseMoveEvent(QMouseEvent *event) {
  // 传递事件
  QOpenGLWindow::mouseMoveEvent(event);
  mouse_pos = event->pos();
  // repaint();
}

// 鼠标滚动事件
void GLCanvas::wheelEvent(QWheelEvent *event) {
  // 传递事件
  QOpenGLWindow::wheelEvent(event);
}

// 键盘按下事件
void GLCanvas::keyPressEvent(QKeyEvent *event) {
  // 传递事件
  QOpenGLWindow::keyPressEvent(event);
}

// 键盘释放事件
void GLCanvas::keyReleaseEvent(QKeyEvent *event) {
  // 传递事件
  QOpenGLWindow::keyReleaseEvent(event);
}

// 取得焦点事件
void GLCanvas::focusInEvent(QFocusEvent *event) {
  // 传递事件
  QOpenGLWindow::focusInEvent(event);
}

// 失去焦点事件
void GLCanvas::focusOutEvent(QFocusEvent *event) {
  // 传递事件
  QOpenGLWindow::focusOutEvent(event);
}

// 调整尺寸事件
void GLCanvas::resizeEvent(QResizeEvent *event) {
  // 传递事件
  QOpenGLWindow::resizeEvent(event);
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

  // XINFO("启用垂直同步");

  // 检查最大ubo size
  int maxUBOSize;
  GLCALL(glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUBOSize));
  XINFO("最大UBO块容量: " + std::to_string(maxUBOSize));

  // 标准混合模式
  GLCALL(glEnable(GL_BLEND));
  GLCALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

  // 初始化渲染管理器
  renderer_manager = new RendererManager(this, 64, 65536);

  // 初始化默认纹理
  load_texture_from_path("../resources/textures/default");

  start_render();
}

void GLCanvas::start_render() {
  // 初始化定时器
  auto render_work = [&]() {
    QElapsedTimer timer;
    timer.start();

    while (!stop_refresh) {
      qint64 start = timer.elapsed();
      // 触发主线程更新
      QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);

      qint64 elapsed = timer.elapsed() - start;
      if (elapsed < des_update_time / 2.0) {
        #ifdef _WIN32
        auto start = std::chrono::high_resolution_clock::now();
    auto end = start + std::chrono::microseconds(int((des_update_time / 2.0 - elapsed) * 1000));
    while (std::chrono::high_resolution_clock::now() < end) {
        std::this_thread::yield(); // 让出CPU时间片
    }
        #else
        usleep((des_update_time / 2.0 - elapsed) * 1000);
        #endif //_WIN32
      }
    }
  };
  update_thread = std::thread(render_work);
  update_thread.detach();
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

void GLCanvas::rendergl() {
  XLogger::glcalls = 0;
  XLogger::drawcalls = 0;
  auto before = std::chrono::high_resolution_clock::now().time_since_epoch();
  // 背景色
  GLCALL(glClearColor(0.23f, 0.23f, 0.23f, 1.0f));
  GLCALL(glClear(GL_COLOR_BUFFER_BIT));

  // 执行渲染
  push_shape();
  renderer_manager->renderAll();

  auto after = std::chrono::high_resolution_clock::now().time_since_epoch();
  pre_frame_time =
      std::chrono::duration_cast<std::chrono::microseconds>(after - before)
          .count();

  // XINFO("frame_time:" + std::to_string(pre_frame_time));

  pre_glcalls = XLogger::glcalls;
  pre_drawcall = XLogger::drawcalls;
  fpsCounter->frameRendered();
}

// 绘制画布
void GLCanvas::paintGL() { rendergl(); }

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

void GLCanvas::add_texture(const char *qrc_path) {
  std::filesystem::path p(qrc_path);
  add_texture(p);
}
// 添加纹理
void GLCanvas::add_texture(std::filesystem::path &path) {
  // 初始化纹理
  auto texture = std::make_shared<TextureInstace>(path);

  // 检查纹理是否已载入过
  auto texit = texture_full_map.find(texture->name);
  if (texit != texture_full_map.end()) {
    // 载入过了
    return;
  }

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

  // 设置引用
  texture->poolreference = pool;

  // 添加到纹理池
  auto res = pool->load_texture(texture);

  if (res) {
    // 成功添加映射
    texture_full_map.try_emplace(texture->name, texture);
  }
}
