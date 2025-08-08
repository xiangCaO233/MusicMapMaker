#include <canvas/map/MapCanvas.hpp>

// 构造MapCanvas
MapCanvas::MapCanvas() {}

// 析构MapCanvas
MapCanvas::~MapCanvas() {}

// 切换到图
void MapCanvas::switch_map(const std::shared_ptr<MMap>& smap) { map = smap; }
