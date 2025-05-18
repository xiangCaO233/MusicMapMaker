#ifndef M_ORBITGENERATOR_H
#define M_ORBITGENERATOR_H

#include <memory>
class MapEditor;

class OrbitGenerator {
   public:
    // 构造OrbitGenerate
    OrbitGenerator(std::shared_ptr<MapEditor> &editor);
    // 析构OrbitGenerate
    virtual ~OrbitGenerator();

    // 编辑器引用
    std::shared_ptr<MapEditor> editor_ref;

    // 生成轨道渲染指令
    void generate();
};

#endif  // M_ORBITGENERATOR_H
