#ifndef MMM_MAPCANVAS_HPP
#define MMM_MAPCANVAS_HPP

#include <canvas/GLCanvas.hpp>
#include <memory>

class MMap;

class MapCanvas : public GLCanvas {
   public:
    // 构造MapCanvas
    MapCanvas();

    // 析构MapCanvas
    ~MapCanvas() override;

    // 切换到图
    void switch_map(const std::shared_ptr<MMap>& smap);

   private:
    // 谱面
    std::weak_ptr<MMap> map;
};
#endif  // MMM_MAPCANVAS_HPP
