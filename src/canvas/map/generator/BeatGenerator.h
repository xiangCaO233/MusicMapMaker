#ifndef M_BEATGENERATOR_H
#define M_BEATGENERATOR_H

#include <QColor>
#include <QRectF>
#include <map>
#include <memory>
#include <queue>

class MapEditor;

struct LineRenderData {
  double x1;
  double y1;
  double x2;
  double y2;

  int32_t r;
  int32_t g;
  int32_t b;
  int32_t a;
  int32_t line_width;
};

struct TimeTextRenderData {
  double x;
  double y;

  std::u32string text;
};

class BeatGenerator {
 public:
  // 构造BeatGenerator
  BeatGenerator(std::shared_ptr<MapEditor> editor);
  // 析构BeatGenerator
  virtual ~BeatGenerator();

  // 节拍线渲染数据队列(确定层级)
  static std::queue<LineRenderData> line_queue;

  // 节拍线渲染数据队列(确定层级)
  static std::queue<TimeTextRenderData> text_queue;

  // 分拍背景渲染数据队列(确定层级)
  static std::queue<QRectF> divbg_queue;

  // 编辑器引用
  std::shared_ptr<MapEditor> editor_ref;

  // 生成拍渲染指令
  void generate();
};

#endif  // M_BEATGENERATOR_H
