#ifndef RENDERCOMMAND_H
#define RENDERCOMMAND_H

#include <glm/vec3.hpp>
#include <vector>

enum Shape { TRIANGLE, QUAD, OVAL };

class RenderCommand {
  // 绘制的图元形状
  Shape instance_shape;
  // 图元顶点列表
  std::vector<glm::vec3> vertices;

 public:
  // 构造RenderCommand
  RenderCommand(Shape shape);
  // 析构RenderCommand
  virtual ~RenderCommand();
};

#endif  // RENDERCOMMAND_H
