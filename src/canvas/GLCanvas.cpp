#include "GLCanvas.h"

#include <GL/gl.h>
#include <qdir.h>
#include <qdiriterator.h>
#include <qlogging.h>
#include <qobject.h>
#include <qpainter.h>

#include <QFile>
#include <QMouseEvent>
#include <QRandomGenerator>
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
  // load_texture_from_path("../resources/textures/test/other",
  //                        TexturePoolType::BASE_POOL, false);
  load_texture_from_path("../resources/textures/test/1024",
                         TexturePoolType::ARRAY, false);
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

void generateRandomQRectF(RendererManager *&renderer_manager,
                          const std::shared_ptr<TextureInstace> &tex,
                          qreal maxX, qreal maxY, qreal maxWidth,
                          qreal maxHeight) {
  // 获取全局随机数生成器
  QRandomGenerator *rand = QRandomGenerator::global();

  // 随机生成左上角坐标(x, y)
  qreal x = rand->bounded(maxX);
  qreal y = rand->bounded(maxY);

  // 随机生成宽度和高度，确保矩形不会超出最大边界
  // qreal width = rand->bounded(maxWidth);
  // qreal height = rand->bounded(maxHeight);
  qreal width = 40;
  qreal height = 40;

  // 确保矩形不会超出最大边界
  // if (x + width > maxX) {
  //  width = maxX - x;
  //}
  // if (y + height > maxY) {
  //  height = maxY - y;
  //}

  QRectF rec(x, y, width, height);
  auto rotation = rand->bounded(360);

  renderer_manager->addRect(rec, tex, Qt::red, 0, true);
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
  auto it = texture_map.begin();
  for (int i = 0; i < 4000; i++) {
    it++;
    if (it == texture_map.end()) {
      it = texture_map.begin();
    }
    generateRandomQRectF(renderer_manager, it->second, 400, 530, 100, 100);
  }

  auto rect = QRectF(100, 100, 300, 300);
  renderer_manager->addRect(rect, texture_map["yuanchou.png"], Qt::red, 0.0f,
                            false);

  auto mouse_rec = QRectF(mouse_pos.x() - 20, mouse_pos.y() - 20, 41, 41);
  renderer_manager->addEllipse(mouse_rec, texture_map["yuanchou.png"],
                               Qt::green, 0.0f, true);

  // 执行渲染
  renderer_manager->renderAll();

  auto after = std::chrono::high_resolution_clock::now().time_since_epoch();
  auto frame_time =
      std::chrono::duration_cast<std::chrono::microseconds>(after - before)
          .count();

  XINFO("frame_time: " + std::to_string(frame_time) + "us");
  XINFO("theoretical fps: " +
        std::to_string(1 / (((float)frame_time) / 1000000.0)) + "fps");

  if (XLogger::glcalls < 100) {
    XINFO("当前帧GLCALL数量: " + std::to_string(XLogger::glcalls));
  } else if (XLogger::glcalls < 500) {
    XWARN("当前帧GLCALL数量: " + std::to_string(XLogger::glcalls));
  } else {
    XCRITICAL("当前帧GLCALL数量: " + std::to_string(XLogger::glcalls));
  }

  if (XLogger::drawcalls < 8) {
    XINFO("当前帧DRAWCALL数量: " + std::to_string(XLogger::drawcalls));
  } else if (XLogger::drawcalls < 32) {
    XWARN("当前帧DRAWCALL数量: " + std::to_string(XLogger::drawcalls));
  } else {
    XCRITICAL("当前帧DRAWCALL数量: " + std::to_string(XLogger::drawcalls));
  }
}

// 从指定目录添加纹理
void GLCanvas::load_texture_from_path(const char *p, TexturePoolType type,
                                      bool use_atlas) {
  QString qps(p);
  QString path = QDir::currentPath() + "/" + qps + "/";
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
      add_texture(file, type, use_atlas);
    }
  }
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
    QImage image(qrc_path);
    pool = creat_new_pool(type, image.size());
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
        int res = pool->load_texture(current_atlas);
        if (res == NOT_CONSECUTIVE) {
          // 使用了纹理数组且id不连续
          // 重新创建纹理池
          pool = creat_new_pool(type, current_atlas->size());
          pool->load_texture(current_atlas);
        }
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
        prepool =
            creat_new_pool(current_atlas_used_pool, current_atlas->size());
      }
      // 载入上一类型的纹理池
      int res = prepool->load_texture(current_atlas);
      // 载入上一类型的纹理池失败
      if (res == NOT_CONSECUTIVE) {
        // 使用了纹理数组且id不连续
        // 重新创建纹理池
        prepool =
            creat_new_pool(current_atlas_used_pool, current_atlas->size());
        prepool->load_texture(current_atlas);
      }
      // 重新初始化纹理集
      current_atlas = std::make_shared<TextureAtlas>();
    }
  } else {
    // 直接初始化纹理
    texture = std::make_shared<TextureInstace>(qrc_path, pool);
    // 载入纹理
    int res = pool->load_texture(texture);
    if (res == NOT_CONSECUTIVE) {
      // 使用了纹理数组且id不连续
      // 重新创建纹理池
      pool = creat_new_pool(type, current_atlas->size());
      pool->load_texture(current_atlas);
    }
  }
  texture_map.try_emplace(texture->name, texture);
}

// 创建新纹理池
std::shared_ptr<BaseTexturePool> GLCanvas::creat_new_pool(
    TexturePoolType type, QSize texture_array_used_size) {
  auto poolsit = renderer_manager->texture_pools.find(type);
  if (poolsit == renderer_manager->texture_pools.end()) {
    // 不存在此类型纹理池列表
    // 新建纹理池列表,更新迭代器
    poolsit = renderer_manager->texture_pools.try_emplace(type).first;
  }
  auto &pools = poolsit->second;
  // 预声明纹理池
  std::shared_ptr<BaseTexturePool> pool;
  switch (type) {
    case TexturePoolType::BASE_POOL: {
      pool = std::make_shared<TexturePool>(this);
      break;
    }
    case TexturePoolType::ARRAY: {
      pool = std::make_shared<TextureArray>(this, texture_array_used_size);
      break;
    }
  }
  pools.emplace_back(pool);
  return pool;
}

// 完成纹理载入
void GLCanvas::finalize_texture_loading() {
  // 如果当前还有未处理的纹理集
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
      prepool = creat_new_pool(current_atlas_used_pool, current_atlas->size());
    }
    // 载入上一类型的纹理池
    int res = prepool->load_texture(current_atlas);
    if (res == NOT_CONSECUTIVE) {
      // 使用了纹理数组且id不连续
      // 重新创建纹理池
      prepool = creat_new_pool(current_atlas_used_pool, current_atlas->size());
      prepool->load_texture(current_atlas);
    }
  }

  for (const auto &[type, pools] : renderer_manager->texture_pools) {
    for (auto &pool : pools) {
      pool->finalize();
    }
  }
}
