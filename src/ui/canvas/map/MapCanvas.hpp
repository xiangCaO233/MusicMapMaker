#ifndef MMM_MAPCANVAS_HPP
#define MMM_MAPCANVAS_HPP

#include <canvas/GLCanvas.hpp>

class MapCanvas : public GLCanvas {
   public:
    // 构造MapCanvas
    MapCanvas();
    // 析构MapCanvas
    ~MapCanvas() override;
};
#endif  // MMM_MAPCANVAS_HPP
