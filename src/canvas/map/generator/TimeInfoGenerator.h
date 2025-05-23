#ifndef M_TIMEINFO_GENERATOR_H
#define M_TIMEINFO_GENERATOR_H

#include <QObject>
#include <memory>

class MapEditor;
class BufferWrapper;

class TimeInfoGenerator {
   public:
    // 构造TimeInfoGenerator
    TimeInfoGenerator(std::shared_ptr<MapEditor>& editor);
    // 析构TimeInfoGenerator
    virtual ~TimeInfoGenerator();

    // 编辑器引用
    std::shared_ptr<MapEditor> editor_ref;

    // 绘制时间点
    void draw_timing_points(BufferWrapper* bufferwrapper);

    // 生成信息
    void generate(BufferWrapper* bufferwrapper);
};

#endif  // M_TIMEINFO_GENERATOR_H
