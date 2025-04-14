#include "MapWorkspaceCanvas.h"

#include <qnamespace.h>

#include <QTimer>

MapWorkspaceCanvas::MapWorkspaceCanvas(QWidget *parent) : GLCanvas(parent) {
  // 添加实时鼠标位置
  live_instances.try_emplace(
      "mouse",
      QRectF(mouse_pos.x() - mouse_size / 2, mouse_pos.y() - mouse_size / 2,
             mouse_size, mouse_size),
      ShapeType::OVAL, TextureEffect::GLOWING);
}

MapWorkspaceCanvas::~MapWorkspaceCanvas() = default;

// qt事件
// 鼠标按下事件
void MapWorkspaceCanvas::mousePressEvent(QMouseEvent *event) {
  // 传递事件
  GLCanvas::mousePressEvent(event);
}

// 鼠标释放事件
void MapWorkspaceCanvas::mouseReleaseEvent(QMouseEvent *event) {
  // 传递事件
  GLCanvas::mouseReleaseEvent(event);
}

// 鼠标双击事件
void MapWorkspaceCanvas::mouseDoubleClickEvent(QMouseEvent *event) {
  // 传递事件
  GLCanvas::mouseDoubleClickEvent(event);
}

// 鼠标移动事件
void MapWorkspaceCanvas::mouseMoveEvent(QMouseEvent *event) {
  // 传递事件
  GLCanvas::mouseMoveEvent(event);
  // 更新实时鼠标位置
  auto &mouse = live_instances["mouse"];
  mouse.bounds.setX(mouse_pos.x() - mouse_size / 2);
  mouse.bounds.setY(mouse_pos.y() - mouse_size / 2);
  mouse.bounds.setWidth(mouse_size);
  mouse.bounds.setHeight(mouse_size);
  // 在鼠标附近更新粒子
}

// 鼠标滚动事件
void MapWorkspaceCanvas::wheelEvent(QWheelEvent *event) {
  // 传递事件
  GLCanvas::wheelEvent(event);
}

// 键盘按下事件
void MapWorkspaceCanvas::keyPressEvent(QKeyEvent *event) {
  // 传递事件
  GLCanvas::keyPressEvent(event);
}

// 键盘释放事件
void MapWorkspaceCanvas::keyReleaseEvent(QKeyEvent *event) {
  // 传递事件
  GLCanvas::keyReleaseEvent(event);
}

// 取得焦点事件
void MapWorkspaceCanvas::focusInEvent(QFocusEvent *event) {
  // 传递事件
  GLCanvas::focusInEvent(event);
}

// 失去焦点事件
void MapWorkspaceCanvas::focusOutEvent(QFocusEvent *event) {
  // 传递事件
  GLCanvas::focusOutEvent(event);
}

// 进入事件
void MapWorkspaceCanvas::enterEvent(QEnterEvent *event) {
  // 传递事件
  GLCanvas::enterEvent(event);
}

// 退出事件
void MapWorkspaceCanvas::leaveEvent(QEvent *event) {
  // 传递事件
  GLCanvas::leaveEvent(event);
}

// 调整尺寸事件
void MapWorkspaceCanvas::resizeEvent(QResizeEvent *event) {
  // 传递事件
  GLCanvas::resizeEvent(event);
}

// 切换到指定图
void MapWorkspaceCanvas::switch_map(std::shared_ptr<MMap> &map) {
  working_map = map;
  repaint();
}
