#ifndef MMM_MPRRIMITIVECOLLECTOR_HPP
#define MMM_MPRRIMITIVECOLLECTOR_HPP

// 图元收集器-析构时自动上传指令列表

#include <glm/fwd.hpp>
class Renderer2D;
// 一级翻译器-将draw调用(参数全,可以以可忽略的时间转换成指令)
// 翻译为gl绘制指令并放入缓存指令列表

class MPrimitiveCollector {
   public:
    // 构造MPrimitiveCollector
    MPrimitiveCollector(Renderer2D* render) : renderer(render) {}

    // 析构MPrimitiveCollector
    virtual ~MPrimitiveCollector();

    // 收集图元渲染指令
    void collect();

   private:
    // 渲染器指针持有
    Renderer2D* renderer;
};
#endif  // MMM_MPRRIMITIVECOLLECTOR_HPP
