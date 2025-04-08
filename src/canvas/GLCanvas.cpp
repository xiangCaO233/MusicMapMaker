#include "GLCanvas.h"

#include <GL/gl.h>
#include <qdir.h>
#include <qdiriterator.h>
#include <qlogging.h>
#include <qpainter.h>

#include <QFile>
#include <QMouseEvent>
#include <QTextStream>
#include <chrono>
#include <memory>
#include <string>

#include "colorful-log.h"
#include "texture/atlas/TextureAtlas.h"
#include "texture/pool/BaseTexturePool.h"
#include "texture/pool/TextureArray.h"
#include "texture/pool/TexturePool.h"

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

GLCanvas::GLCanvas(QWidget *parent) : QOpenGLWidget(parent) {
  // 启用鼠标跟踪
  setMouseTracking(true);
  // setUpdateBehavior(QOpenGLWidget::PartialUpdate);
}

GLCanvas::~GLCanvas() {
  // 释放渲染管理器
  delete renderer_manager;
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
  auto mouse_rec = QRectF(mouse_pos.x() - 10, mouse_pos.y() - 10, 20, 20);
  renderer_manager->addEllipse(mouse_rec, texture_map["yuanchou.png"],
                               Qt::green, 0.0f, true);
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
  TextureArray::max_texture_layer = maxLayers;

  // 查询纹理采样器最大连续数量
  GLint max_fragment_samplers;
  glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_fragment_samplers);
  XINFO("纹理采样器最大连续数量: " + std::to_string(max_fragment_samplers / 2));
  if (max_fragment_samplers > 16) {
    max_fragment_samplers = 16;
  }
  TexturePool::max_sampler_consecutive_count = max_fragment_samplers;

  // 查询纹理采样器最大数量
  GLint max_combined_samplers;
  glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_combined_samplers);
  XINFO("纹理采样器最大数量: " + std::to_string(max_combined_samplers));
  TexturePool::max_total_sampler_count = max_combined_samplers;

  // 查询最大支持抗锯齿MSAA倍率
  GLint maxSamples;
  GLCALL(glGetIntegerv(GL_MAX_SAMPLES, &maxSamples));

  XINFO("启用最大抗锯齿倍率: " + std::to_string(maxSamples));
  // 启用 最大 MSAA
  context()->format().setSamples(maxSamples);

  XINFO("启用垂直同步");
  // 启用V-Sync
  context()->format().setSwapInterval(1);

  // 检查最大ubo size
  int maxUBOSize;
  GLCALL(glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUBOSize));
  XINFO("最大UBO块容量: " + std::to_string(maxUBOSize));

  // 标准混合模式
  GLCALL(glEnable(GL_BLEND));
  GLCALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
  // 初始化渲染管理器
  renderer_manager = new RendererManager(this, 64, 4096);
  QString path = QDir::currentPath() + "/../resources/textures/test/1024/";

  QDir dir(path);
  if (!dir.exists()) {
    qWarning() << "Directory does not exist:" << path;
    return;
  }

  // 递归遍历所有文件和子目录
  QDirIterator it(path, QDirIterator::Subdirectories);
  while (it.hasNext()) {
    QString filePath = it.next();
    std::string filestr = filePath.toStdString();
    auto file = filestr.c_str();
    if (it.fileInfo().isFile()) {
      add_texture(file, TexturePoolType::BASE_POOL, false);
    }
  }
  finalize_texture_loading();
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
  renderer_manager->set_uniform_mat4("projection_mat", proj);
}

// 绘制画布
void GLCanvas::paintGL() {
  XLogger::glcalls = 0;
  XLogger::drawcalls = 0;
  auto before = std::chrono::high_resolution_clock::now().time_since_epoch();
  // 背景色
  GLCALL(glClearColor(0.23f, 0.23f, 0.23f, 1.0f));
  GLCALL(glClear(GL_COLOR_BUFFER_BIT));

  // 添加渲染内容
  auto rect = QRectF(100, 100, 50, 50);
  renderer_manager->addRect(rect, texture_map["yuanchou.png"], Qt::red, 15.0f,
                            false);

  auto rect3 = QRectF(50, 200, 80, 160);
  renderer_manager->addRect(rect3, texture_map["xinzexi.png"], Qt::cyan, -30.0f,
                            false);

  auto rect4 = QRectF(200, 60, 75, 30);
  renderer_manager->addRect(rect4, texture_map["aijier.png"], Qt::yellow, 0.0f,
                            false);

  auto rect2 = QRectF(0, 0, 100, 100);
  renderer_manager->addRect(rect2, texture_map["xingdengbao.png"], Qt::blue,
                            45.0f, false);

  // 执行渲染
  renderer_manager->renderAll();
  auto after = std::chrono::high_resolution_clock::now().time_since_epoch();
  auto frame_time =
      std::chrono::duration_cast<std::chrono::nanoseconds>(after - before);
  XINFO("frame_time: " + std::to_string(frame_time.count()) + "ns");
  XWARN("当前帧GLCALL数量: " + std::to_string(XLogger::glcalls));
  XWARN("当前帧DRAWCALL数量: " + std::to_string(XLogger::drawcalls));
}
// 设置垂直同步
void GLCanvas::set_Vsync(bool flag) {
  // 切换V-Sync
  context()->format().setSwapInterval(flag ? 1 : 0);
}

// 添加纹理
void GLCanvas::add_texture(const char *qrc_path, TexturePoolType type,
                           bool use_atlas) {
  auto poolsit = renderer_manager->texture_pools.find(type);
  if (poolsit == renderer_manager->texture_pools.end()) {
    // 不存在此类型纹理池列表
    // 新建纹理池列表,更新迭代器
    poolsit = renderer_manager->texture_pools.try_emplace(type).first;
  }
  auto &pools = poolsit->second;
  std::shared_ptr<BaseTexturePool> pool;

  // 检查是否需要新建纹理池
  bool need_new_pool{true};
  if (!pools.empty()) {
    for (const auto &current_pool : pools) {
      if (!current_pool->is_full()) {
        // 包含未满的纹理池,使用此纹理池
        need_new_pool = false;
        pool = current_pool;
        break;
      }
    }
  }
  if (need_new_pool) {
    // 新建纹理池
    switch (type) {
      case TexturePoolType::BASE_POOL: {
        pool = std::make_shared<TexturePool>(this);
        break;
      }
      case TexturePoolType::ARRARY: {
        QImage image(qrc_path);
        pool = std::make_shared<TextureArray>(this, image.size());
        break;
      }
    }
    // 添加纹理池
    pools.push_back(pool);
  }

  // 加载纹理
  std::shared_ptr<TextureInstace> texture;
  if (use_atlas) {
    // 使用纹理集
    if (!current_atlas) {
      // 初始化纹理集
      current_atlas = std::make_shared<TextureAtlas>();
      current_atlas_used_pool = type;
    }
    if (type == current_atlas_used_pool) {
      // 使用相同类型纹理池
      if (current_atlas->is_full()) {
        // 当前纹理集已满
        // 载入纹理池
        current_atlas->pack();
        pool->load_texture(current_atlas);
        // 重新初始化纹理集
        current_atlas = std::make_shared<TextureAtlas>();
      } else {
        // 当前纹理集未满
        // 加入当前纹理集
        current_atlas->add_texture(qrc_path);
      }

    } else {
      // 使用不同类型
      // 打包当前纹理集
      current_atlas->pack();

      // 检查前一类型纹理池
      auto prepoolsit =
          renderer_manager->texture_pools.find(current_atlas_used_pool);
      auto &prepools = prepoolsit->second;
      std::shared_ptr<BaseTexturePool> prepool;
      // 检查是否需要新建纹理池
      bool prefull{true};
      if (!prepools.empty()) {
        for (const auto &pre_pool : prepools) {
          if (!pre_pool->is_full()) {
            // 包含未满的纹理池,使用此纹理池
            prefull = false;
            prepool = pre_pool;
            break;
          }
        }
      }
      if (prefull) {
        // 新建纹理池
        switch (current_atlas_used_pool) {
          case TexturePoolType::BASE_POOL: {
            prepool = std::make_shared<TexturePool>(this);
            break;
          }
          case TexturePoolType::ARRARY: {
            prepool =
                std::make_shared<TextureArray>(this, current_atlas->size());
            break;
          }
        }
        // 添加纹理池
        prepools.push_back(prepool);
      }
      // 载入上一类型的纹理池
      prepool->load_texture(current_atlas);
      // 重新初始化纹理集
      current_atlas = std::make_shared<TextureAtlas>();
    }
  } else {
    // 直接初始化纹理
    texture = std::make_shared<TextureInstace>(qrc_path, pool);
    // 载入纹理
    auto res = pool->load_texture(texture);
  }
  texture_map.try_emplace(texture->name, texture);
}

// 完成纹理载入
void GLCanvas::finalize_texture_loading() {
  if (current_atlas) {
    // 打包当前纹理集
    current_atlas->pack();
    // 检查前一类型纹理池
    auto prepoolsit =
        renderer_manager->texture_pools.find(current_atlas_used_pool);
    auto &prepools = prepoolsit->second;
    std::shared_ptr<BaseTexturePool> prepool;
    // 检查是否需要新建纹理池
    bool prefull{true};
    if (!prepools.empty()) {
      for (const auto &pre_pool : prepools) {
        if (!pre_pool->is_full()) {
          // 包含未满的纹理池,使用此纹理池
          prefull = false;
          prepool = pre_pool;
          break;
        }
      }
    }
    if (prefull) {
      // 新建纹理池
      switch (current_atlas_used_pool) {
        case TexturePoolType::BASE_POOL: {
          prepool = std::make_shared<TexturePool>(this);
          break;
        }
        case TexturePoolType::ARRARY: {
          prepool = std::make_shared<TextureArray>(this, current_atlas->size());
          break;
        }
      }
      // 添加纹理池
      prepools.push_back(prepool);
    }
    // 载入上一类型的纹理池
    prepool->load_texture(current_atlas);
  }
  for (const auto &[type, pools] : renderer_manager->texture_pools) {
    for (auto &pool : pools) {
      pool->finalize();
    }
  }
}
