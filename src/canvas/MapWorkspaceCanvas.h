#ifndef M_MAPWORKSPACE_H
#define M_MAPWORKSPACE_H

#include <cstdint>
#include <memory>

#include "GLCanvas.h"
#include "src/mmm/map/MMap.h"

class MapWorkspaceCanvas : public GLCanvas {
 protected:
  // qt事件
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseDoubleClickEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
  void keyReleaseEvent(QKeyEvent *event) override;
  void focusInEvent(QFocusEvent *event) override;
  void focusOutEvent(QFocusEvent *event) override;
  void enterEvent(QEnterEvent *event) override;
  void leaveEvent(QEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;

 public:
  // 构造MapWorkspaceCanvas
  explicit MapWorkspaceCanvas(QWidget *parent = nullptr);

  // 析构MapWorkspaceCanvas
  ~MapWorkspaceCanvas() override;

  // 光标大小
  float mouse_size{24};

  // 正在工作的图
  std::shared_ptr<MMap> working_map;
  uint64_t current_time_stamp;

  // 切换到指定图
  void switch_map(std::shared_ptr<MMap> &map);
};

#endif  // M_MAPWORKSPACE_H
