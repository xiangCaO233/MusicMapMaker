#include <qalgorithms.h>

#include <GLCanvas.hpp>
#include <canvas/map/MapCanvas.hpp>
#include <info/MapCanvasInfo.hpp>
#include <mmm/map/MMap.hpp>

// 构造MapCanvas
MapCanvas::MapCanvas() : GLCanvas() {
    // 初始化共享信息
    initSharedInfo<MapCanvasInfo>();
}

// 析构MapCanvas
MapCanvas::~MapCanvas() { qDeleteAll(tools); }

void MapCanvas::initializeGL() {
    GLCanvas::initializeGL();
    // 初始化默认皮肤
    skin = editor_skins
               .try_emplace("Default",
                            std::make_unique<MSkin>("", textureCallback()))
               .first->second.get();
}

// 切换到图
void MapCanvas::switch_map(MMap* smap) {
    map = smap;
    auto mapcanvasInfo = info<MapCanvasInfo>();
    mapcanvasInfo->mapInfo.cover_path =
        smap->base_metadata().main_cover_path.generic_string();
    update_sharedInfo();
}

// 创建工具
void MapCanvas::creatTools() {}
