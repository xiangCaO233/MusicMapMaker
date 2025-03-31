#ifndef GLRENDERER_H
#define GLRENDERER_H

#include "RenderCommand.h"
class GLRenderer {
 public:
  // 构造GLRenderer
  GLRenderer();
  // 析构GLRenderer
  virtual ~GLRenderer();

  // 发送绘制指令
  void send_command(RenderCommand &command);
};

#endif  // GLRENDERER_H
