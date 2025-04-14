#ifndef M_MAPWORKSPACE_H
#define M_MAPWORKSPACE_H

#include "GLCanvas.h"

class MapWorkspaceCanvas : public GLCanvas {
 public:
  // 构造MapWorkspaceCanvas
  explicit MapWorkspaceCanvas(QWidget *parent = nullptr);

  // 析构MapWorkspaceCanvas
  ~MapWorkspaceCanvas() override;
};

#endif  // M_MAPWORKSPACE_H
