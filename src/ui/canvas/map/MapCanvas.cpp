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
    skin =
        editor_skins
            .try_emplace("Default", std::make_unique<MSkin>(
                                        "../resources/textures/default",
                                        audioLoadCallback(), textureCallback()))
            .first->second.get();
    emit skinInitialized();
}

// 绑定音频载入回调
void MapCanvas::onAudioLoadcbkInitialized(AudioLoadCallback* cbk) {
    GLCanvas::onAudioLoadcbkInitialized(cbk);
    // 载入全部默认皮肤的音效
    for (const auto& [type, soundRpath] : skin->sound_effects) {
        auto soundApath = skin->skinPath / soundRpath;
        cbk->loadBack(soundApath.generic_string());
    }
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
