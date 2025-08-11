#include <qalgorithms.h>

#include <canvas/map/MapCanvas.hpp>

#include "GLCanvas.hpp"
#include "info/MapCanvasInfo.hpp"

// 构造MapCanvas
MapCanvas::MapCanvas() : GLCanvas() {
    // 初始化共享信息
    initSharedInfo<MapCanvasInfo>();
}

// 析构MapCanvas
MapCanvas::~MapCanvas() { qDeleteAll(tools); }

// 切换到图
void MapCanvas::switch_map(const std::shared_ptr<MMap>& smap) { map = smap; }

// 创建工具
void MapCanvas::creatTools() {}
