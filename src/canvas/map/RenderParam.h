#ifndef M_RENDER_PARAMETERS_H
#define M_RENDER_PARAMETERS_H

#include <memory>
#include <queue>

class TextureInstace;

enum class RenderType {
  BG,
  BEAT,
  INFO,
  TOPBAR,
  SELECTION,
  HITOBJECT,
  PREVIEW,
};

enum class FunctionType {
  RECT,
  ROUNDRECT,
  ELLIPSE,
  LINE,
  TEXT,
};

struct RenderParams {
  FunctionType func_type;
  unsigned char r;
  unsigned char g;
  unsigned char b;
  unsigned char a;
  double xpos;
  double ypos;
  double width;
  double height;
  double rotation;
  double radius;
  std::shared_ptr<TextureInstace> texture;
  std::u32string str;
};

struct RenderParamsBundle {
  std::queue<RenderParams> params_bundle;
};

#endif  // M_RENDER_PARAMETERS_H
