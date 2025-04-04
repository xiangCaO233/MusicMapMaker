#include "GLCanvas.h"

#include <GL/gl.h>
#include <qlogging.h>
#include <qpainter.h>

#include <QFile>
#include <QMouseEvent>
#include <QTextStream>
#include <string>

#include "colorful-log.h"

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
  XINFO("OpenGL Version: " +
        std::string(reinterpret_cast<const char *>(version)));
  // 查询最大支持抗锯齿MSAA倍率
  GLint maxSamples;
  GLCALL(glGetIntegerv(GL_MAX_SAMPLES, &maxSamples));

  XINFO("启用最大抗锯齿倍率: " + std::to_string(maxSamples));
  // 启用 最大 MSAA
  context()->format().setSamples(maxSamples);

  XINFO("启用垂直同步");
  // 启用V-Sync
  context()->format().setSwapInterval(1);

  // 标准混合模式
  GLCALL(glEnable(GL_BLEND));
  GLCALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
  GLuint VAO;
  GLCALL(glGenVertexArrays(1, &VAO));
  // 初始化渲染管理器
  renderer_manager = new RendererManager(this, 64, 4096);
}
void GLCanvas::resizeGL(int w, int h) {
  GLCALL(glViewport(0, 0, w, h));

  // 投影矩阵
  QMatrix4x4 proj;
  // 计算正交投影矩阵
  proj.ortho(-(float)w / 2.0f, (float)w / 2.0f, -(float)h / 2.0f,
             (float)h / 2.0f, -1.0f, 1.0f);
  // proj.ortho(0.0f, w, h, 0.0f, -1.0f, 1.0f);
  // 反转y轴
  proj.scale(1.0f, -1.0f, 1.0f);
  // proj.translate(-0.5f, -0.5f, 0.0f);
  // proj.transposed();

  // 更新uniform
  renderer_manager->set_uniform_mat4("projection_mat", proj);
}

// 绘制画布
void GLCanvas::paintGL() {
  XLogger::glcalls = 0;
  // 背景色
  GLCALL(glClearColor(0.23f, 0.23f, 0.23f, 1.0f));
  GLCALL(glClear(GL_COLOR_BUFFER_BIT));

  // 添加矩形渲染
  auto rect = QRectF(100, 100, 50, 50);
  renderer_manager->addRect(rect, nullptr, Qt::red, false);

  auto rect3 = QRectF(50, 200, 80, 160);
  renderer_manager->addRect(rect3, nullptr, Qt::cyan, false);

  auto rect2 = QRectF(0, 0, 100, 100);
  renderer_manager->addRect(rect2, nullptr, Qt::blue, true);

  // 执行渲染
  renderer_manager->renderAll();
  XWARN("当前帧GLCALL数量: " + std::to_string(XLogger::glcalls));
}
// 设置垂直同步
void GLCanvas::set_Vsync(bool flag) {
  // 切换V-Sync
  context()->format().setSwapInterval(flag ? 1 : 0);
}
