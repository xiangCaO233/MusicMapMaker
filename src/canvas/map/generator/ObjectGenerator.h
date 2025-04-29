#ifndef M_OBJECTGENERATOR_H
#define M_OBJECTGENERATOR_H

#include <QRectF>
#include <map>
#include <memory>
#include <queue>

class MapEditor;
class HitObject;
class Note;
class Hold;
class Slide;
class TextureInstace;

struct ObjectRenderData {
  double x;
  double y;
  double w;
  double h;
  std::shared_ptr<TextureInstace> tex;
  std::shared_ptr<HitObject> objref;
};

class ObjectGenerator {
 public:
  // 构造ObjectGenerator
  explicit ObjectGenerator(std::shared_ptr<MapEditor>& editor);

  // 析构ObjectGenerator
  virtual ~ObjectGenerator();

  // 图形渲染数据队列(确定层级)
  static std::queue<ObjectRenderData> shape_queue;

  // 共用缓存节点映射表
  static std::map<std::shared_ptr<HitObject>, QRectF> temp_node_map;

  // 编辑器引用
  std::shared_ptr<MapEditor> editor_ref;

  // 转移全部缓存节点到队列中
  void dump_nodes_to_queue();

  // 生成物件渲染指令
  void generate(const std::shared_ptr<HitObject>& hitobject);

  virtual void generate(Note& note);
  virtual void generate(Hold& hold);
  virtual void generate(Slide& slide);

  // 完成生成-添加到渲染列表
  virtual void object_enqueue() = 0;
};

#endif  // M_OBJECTGENERATOR_H
