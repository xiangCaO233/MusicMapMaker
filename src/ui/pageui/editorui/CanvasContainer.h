#ifndef M_CANVASCONTAINER_H
#define M_CANVASCONTAINER_H

#include <QWidget>

#include "MapWorkspaceCanvas.h"

class CanvasContainer : public QWidget {
 public:
  // 构造CanvasContainer
  CanvasContainer(QWidget* parent = nullptr);
  // 析构CanvasContainer
  ~CanvasContainer() override;

  // 画布本体
  QScopedPointer<MapWorkspaceCanvas> canvas;

  // 用于嵌入 QOpenGLWindow
  QWidget* container;

 protected:
};

#endif  // M_CANVASCONTAINER_H
