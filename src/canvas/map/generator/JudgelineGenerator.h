#ifndef M_JUDGELINEGENERATOR_H
#define M_JUDGELINEGENERATOR_H

#include <memory>
class MapEditor;

class JudgelineGenerator {
   public:
    // 构造JudgelineGenerator
    JudgelineGenerator(std::shared_ptr<MapEditor> &editor);
    // 析构JudgelineGenerator
    virtual ~JudgelineGenerator();

    // 编辑器引用
    std::shared_ptr<MapEditor> editor_ref;

    // 生成轨道渲染指令
    void generate();
};

#endif  // M_JUDGELINEGENERATOR_H
