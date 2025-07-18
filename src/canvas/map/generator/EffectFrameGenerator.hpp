#ifndef M_EFFECTGENERATOR_H
#define M_EFFECTGENERATOR_H

#include <memory>

class MapEditor;

class EffectGenerator {
   public:
    // 构造EffectGenerator
    EffectGenerator(std::shared_ptr<MapEditor> editor);
    // 析构EffectGenerator
    virtual ~EffectGenerator();

    // 编辑器引用
    std::shared_ptr<MapEditor> editor_ref;

    // 生成轨道渲染指令
    void generate();
};

#endif  // M_EFFECTGENERATOR_H
