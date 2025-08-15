#include <qalgorithms.h>

#include <GLCanvas.hpp>
#include <canvas/map/MapCanvas.hpp>
#include <info/MapCanvasInfo.hpp>
#include <layer/MapLayerManager.hpp>
#include <mmm/map/MMap.hpp>
#include <render/synchronize/tick/map/MapDataLoop.hpp>
#include <tool/note/NoteTool.hpp>
#include <tool/select/SelectTool.hpp>

// 构造MapCanvas
MapCanvas::MapCanvas() : GLCanvas() {
    // 初始化共享信息
    initSharedInfo<MapCanvasInfo>();
    // 初始化工具
    creatTools();
}

// 析构MapCanvas
MapCanvas::~MapCanvas() { qDeleteAll(tools); }

void MapCanvas::initializeGL() {
    GLCanvas::initializeGL();
    // 初始化渲染数据循环
    dataloop() = std::make_unique<MapDataLoop>(renderer().get());
    dataloop()->initializeLayerManager();
    dataloop()->set_targetFPS(desired_fps());
    connect(dataloop().get(), &RenderDataLoop::renderUpdate, this,
            qOverload<>(&QOpenGLWindow::update));
    connect(fps_counter(), &FrameRateCounter::fpsUpdated, dataloop().get(),
            &RenderDataLoop::updateFPS);
    dataloop()->start();

    // 初始化默认皮肤
    skin = editor_skins
               .try_emplace("Default-Nagisssa",
                            std::make_unique<MSkin>(
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
    static_cast<MapLayerManager*>(dataloop()->layermanager())->updateMap(smap);
    auto mapcanvasInfo = info<MapCanvasInfo>();
    mapcanvasInfo->mapInfo.cover_path =
        smap->base_metadata().main_cover_path.generic_string();
    update_sharedInfo();
}

// 使用工具
void MapCanvas::use_tool(const QString& tool_name) {
    auto toolit = tools.find(tool_name);
    if (toolit != tools.end()) {
        current_tool = toolit.value();
    }
}

// 创建工具
void MapCanvas::creatTools() {
    // 默认使用选择工具
    current_tool =
        tools.try_emplace("Select", new SelectTool(this)).first->second;
    // 创建物件工具
    tools.try_emplace("Note", new NoteTool(this));
}
