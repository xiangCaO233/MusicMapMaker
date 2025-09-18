#include <audio/control/audiocontroller.h>
#include <qalgorithms.h>

#include <GLCanvas.hpp>
#include <action/modules/canvas/EditorActionHandler.hpp>
#include <canvas/map/MapCanvas.hpp>
#include <info/MapCanvasInfo.hpp>
#include <layer/MapLayerManager.hpp>
#include <mmm/map/MMap.hpp>
#include <mmm/project/MProject.hpp>
#include <render/synchronize/tick/map/MapDataLoop.hpp>
#include <tool/note/NoteTool.hpp>
#include <tool/select/SelectTool.hpp>

// 构造MapCanvas
MapCanvas::MapCanvas() : GLCanvas() {
    // 初始化共享信息
    initSharedInfo<MapCanvasInfo>();
    // 初始化播放回调
    maintrack_callback = std::make_shared<CanvasAudioPlayCallback>(this);

    auto thiscp = this;

    // 连接信号
    connect(EditorActionHandler::instance(),
            &EditorActionHandler::pause_or_resume_canvas, [thiscp]() {
                auto maintrack = thiscp->map->base_metadata().main_audio_path;
                auto controller = thiscp->audio_callback->getController(
                    maintrack.generic_string());
                auto sourcenode = controller->node();
                auto mapinfo = thiscp->info<MapCanvasInfo>();
                if (auto node = sourcenode.lock()) {
                    // 切换播放状态
                    node->isplaying() ? node->pause() : node->play();
                    mapinfo->realTimeInfo.is_playing = node->isplaying();
                }
            });
}

// 析构MapCanvas
MapCanvas::~MapCanvas() {
    release_threads();
    release_render();
    qDeleteAll(tools);
}

void MapCanvas::initializeGL() {
    GLCanvas::initializeGL();
    // 初始化渲染数据循环
    dataloop() = std::make_unique<MapDataLoop>(renderer().get());
    dataloop()->initializeLayerManager();
    dataloop()->set_targetFPS(desired_fps());
    connect(dataloop().get(), &RenderDataLoop::renderUpdate, this,
            qOverload<>(&QOpenGLWindow::update));

    // 初始化默认皮肤
    skin = editor_skins
               .try_emplace("Default-Nagisssa",
                            std::make_unique<MSkin>(
#ifdef __APPLE__
                                "../../../../resources/textures/default",
#else
                                "../resources/textures/default",
#endif  //__APPLE__
                                audioLoadCallback(), textureCallback()))
               .first->second.get();
    info<MapCanvasInfo>()->editorInfo.skin = skin;
    update_sharedInfo();
    emit skinInitialized();
    dataloop()->start();

    auto layermanager =
        static_cast<MapLayerManager*>(dataloop()->layermanager());
    // 创建工具
    creatTools(layermanager->get_tool_system(), layermanager->get_tool_cmdq(),
               layermanager->get_tool_interaction_state());
}

// 绑定音频载入回调
void MapCanvas::onAudioLoadcbkInitialized(AudioLoadCallback* cbk) {
    GLCanvas::onAudioLoadcbkInitialized(cbk);
    // 载入全部默认皮肤的音效
    for (const auto& [type, soundRpath] : skin->sound_effects) {
        auto soundApath = skin->skinPath / soundRpath;
        auto weak_track = cbk->loadBack(soundApath.generic_string());

        // std::thread t([cbk]() {
        //     while (true) {
        //         using namespace std::chrono_literals;
        //         std::this_thread::sleep_for(10ms);
        //         cbk->play_oneshot(
        //             "/home/xiang/Documents/coding/c_cpp/MusicMapMaker/"
        //             "resources/"
        //             "textures/default/打击特效/key音/单键.wav");
        //     }
        // });

        // t.detach();
    }
    audio_callback = cbk;
    info<MapCanvasInfo>()->audio_callback = cbk;
}

void MapCanvas::onUpdateTexinfo() {
    // 清理皮肤缓存
    qDebug() << "纹理重组,清理皮肤缓存";
    skin->clear_buffer();
}

// 切换到图
void MapCanvas::switch_map(MMap* smap) {
    map = smap;
    static_cast<MapLayerManager*>(dataloop()->layermanager())->updateMap(smap);
    auto mapcanvasInfo = info<MapCanvasInfo>();
    mapcanvasInfo->bindProjectConfig(smap->project()->cfg());

    mapcanvasInfo->mapInfo.cover_path =
        smap->base_metadata().main_cover_path.generic_string();
    mapcanvasInfo->editorInfo.map = smap;

    // 绑定播放回调
    auto controller = audioLoadCallback()->getController(
        smap->base_metadata().main_audio_path.generic_string());
    controller->add_playcallback(maintrack_callback);

    // 连接暂停按钮信号
    connect(controller->pause_button(), &QPushButton::toggled,
            [mapcanvasInfo](bool checked) {
                mapcanvasInfo->realTimeInfo.is_playing = !checked;
            });

    // 连接调速滑块信号
    connect(controller->speed_slider(), &QSlider::valueChanged,
            [mapcanvasInfo](int value) {
                mapcanvasInfo->realTimeInfo.audio_playback_rate.store(
                    double(value) / 10000.);
            });
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
void MapCanvas::creatTools(ToolSystem* toolsystem,
                           ThreadSafeQueue<ToolCommand>* const cmdq,
                           ToolInteractionState* const toolIntState) {
    // 默认使用选择工具
    current_tool =
        tools
            .try_emplace("Select",
                         new SelectTool(this, toolsystem, cmdq, toolIntState))
            .first->second;
    // 创建物件工具
    tools.try_emplace("Note",
                      new NoteTool(this, toolsystem, cmdq, toolIntState));

    current_tool = tools["Note"];
}
