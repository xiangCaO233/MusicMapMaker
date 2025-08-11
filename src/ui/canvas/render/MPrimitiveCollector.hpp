#ifndef MMM_MPRRIMITIVECOLLECTOR_HPP
#define MMM_MPRRIMITIVECOLLECTOR_HPP

// 图元收集器-析构时自动上传指令列表

#include <layer/ILayer.hpp>

class Renderer2D;
class LayerManager;
class MPrimitiveCollector {
   public:
    // 构造MPrimitiveCollector
    explicit MPrimitiveCollector(Renderer2D* render, LayerManager* manager);

    // 析构MPrimitiveCollector
    ~MPrimitiveCollector();

    // 收集图元渲染指令
    void collect();

   private:
    // 渲染器指针持有
    Renderer2D* renderer;
    LayerManager* layer_manager;
};
#endif  // MMM_MPRRIMITIVECOLLECTOR_HPP
