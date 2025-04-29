#ifndef M_AREAINFOGENERATOR_H
#define M_AREAINFOGENERATOR_H

#include <memory>

class MapEditor;

// 当前画布时间区域信息生成器

class AreaInfoGenerator {
 public:
  // 构造AreaInfoGenerator
  AreaInfoGenerator(std::shared_ptr<MapEditor> editor);
  // 析构AreaInfoGenerator
  virtual ~AreaInfoGenerator();

  // 编辑器引用
  std::shared_ptr<MapEditor> editor_ref;

  // 生成信息
  void generate();
};

#endif  // M_AREAINFOGENERATOR_H
