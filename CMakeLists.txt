cmake_minimum_required(VERSION 3.28..4.0)

project(
  MusicMapMaker
  VERSION 0.1
  LANGUAGES CXX)

if(MSVC)
  add_compile_options(/Zc:__cplusplus)
  add_compile_options(/utf-8 /EHsc)
endif()

add_subdirectory(./libs/IonCachyEngine)

include_directories(./ include/ ./libs/IonCachyEngine/include)

link_directories(./build)

# 启用自动处理 Qt 特性
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

# 查找 Qt 包
find_package(
  Qt6 REQUIRED
  COMPONENTS Core
             Concurrent
             Widgets
             OpenGLWidgets
             OpenGL
             LinguistTools
             Svg
             SvgWidgets)

# 查找日志包
find_package(spdlog REQUIRED)

# 傅里叶变换包
find_package(FFTW3 REQUIRED)

# gl数学包
find_package(glm REQUIRED)

if(APPLE)
  set(FFTLIB /opt/homebrew/lib/libfftw3f.dylib /opt/homebrew/lib/libfftw3f.a)
else()
  set(FFTLIB fftw3f)
endif()

# 查找字体包
find_package(Freetype REQUIRED)

# 查找pugixml包
find_package(pugixml REQUIRED)

# 字符编码包
find_package(ICU REQUIRED COMPONENTS uc i18n data)

# 设置mmm相关文件
set(MMM_SOURCES
    src/mmm/info/osu/OsuMapInfo.cpp
    src/mmm/project/MProject.cpp
    src/mmm/obj/Note.cpp
    src/mmm/obj/rm/Composite.cpp
    src/mmm/obj/osu/OsuNote.cpp
    src/mmm/obj/osu/OsuHold.cpp
    src/mmm/timing/osu/OsuTiming.cpp
    src/mmm/map/MMap.cpp
    src/mmm/map/editor/MMapEditor.cpp
    src/mmm/map/io/malody/MalodyMapRead.cpp
    src/mmm/map/io/malody/MalodyMapWrite.cpp
    src/mmm/map/io/osu/OsuMapRead.cpp
    src/mmm/map/io/osu/OsuMapWrite.cpp
    src/mmm/map/io/mmm/MMapRead.cpp
    src/mmm/map/io/mmm/MMapWrite.cpp
    src/mmm/map/io/rm/RMMapRead.cpp
    src/mmm/map/io/rm/RMMapWrite.cpp)

# 设置翻译文件
set(TS_FILES translations/MusicMapMaker_zh_CN.ts
             translations/MusicMapMaker_en_US.ts)

# 设置着色器文件
set(GLSL_FILES
    resources/glsl/canvas/quad_fshader.glsl.frag
    resources/glsl/canvas/quad_vshader.glsl.vert
    resources/glsl/spectro_fshader.glsl.frag
    resources/glsl/spectro_vshader.glsl.vert
    resources/glsl/wavgraph_fshader.glsl.frag
    resources/glsl/wavgraph_vshader.glsl.vert)

set(CANVAS_SOURCES
    src/ui/canvas/render/texture/font/FontPool.cpp
    src/ui/canvas/render/texture/TexturePool.cpp
    src/ui/canvas/render/command/GLDirectPainter.cpp
    src/ui/canvas/render/command/MCommandCollector.cpp
    src/ui/canvas/render/pipeline/framebuffer/FrameBuffer.cpp
    src/ui/canvas/render/Renderer2D.hpp
    src/ui/canvas/render/pipeline/Renderer2D.cpp
    src/ui/canvas/render/pipeline/Renderer2DGLRess.cpp
    src/ui/canvas/render/pipeline/QuadRenderer2DRess.cpp
    src/ui/canvas/render/pipeline/MeshRenderer2DRess.cpp
    src/ui/canvas/render/pipeline/PrimitiveRenderer2DRess.cpp
    src/ui/canvas/render/pipeline/CurveRenderer2DRess.cpp
    src/ui/canvas/render/pipeline/AfterEffectRenderer2DRess.cpp
    src/ui/canvas/render/synchronize/FrameSynchronizer.cpp
    src/ui/canvas/render/synchronize/tick/RenderDataLoop.cpp
    src/ui/canvas/render/synchronize/tick/RenderLoopWorker.cpp
    src/ui/canvas/render/synchronize/tick/map/MapDataLoop.cpp
    src/ui/canvas/FrameRateCounter.cpp
    src/ui/canvas/GLCanvas.cpp
    src/ui/canvas/tool/BaseTool.cpp
    src/ui/canvas/tool/ToolInteractionState.cpp
    src/ui/canvas/tool/select/SelectTool.cpp
    src/ui/canvas/tool/note/NoteTool.cpp
    src/ui/canvas/ecs/ECSCore.cpp
    src/ui/canvas/ecs/system/sync/SyncEditStatus.cpp
    src/ui/canvas/ecs/system/sync/SyncEntities.cpp
    src/ui/canvas/ecs/system/sync/SyncEffects.cpp
    src/ui/canvas/ecs/system/sync/SyncToolInteractions.cpp
    src/ui/canvas/layer/LayerComputerBase.cpp
    src/ui/canvas/layer/MapLayerManager.cpp
    src/ui/canvas/layer/background/BackgroundLayer.cpp
    src/ui/canvas/layer/background/BackgroundLayerGenerator.cpp
    src/ui/canvas/layer/effect/EffectLayer.cpp
    src/ui/canvas/layer/effect/EffectLayerGenerator.cpp
    src/ui/canvas/layer/interact/InteractLayerGenerator.cpp
    src/ui/canvas/layer/interact/RealTimeInteractLayer.cpp
    src/ui/canvas/layer/note/NoteLayer.cpp
    src/ui/canvas/layer/note/NoteLayerGenerator.cpp
    src/ui/canvas/layer/timeline/TimelineLayer.cpp
    src/ui/canvas/layer/timeline/TimelineLayerGenerator.cpp
    src/ui/canvas/map/MapCanvas.cpp
    src/ui/canvas/map/MapCanvasActions.cpp
    src/ui/canvas/map/MapCanvasEvent.cpp
    src/ui/canvas/map/skin/MSkin.cpp
    src/ui/canvas/GLCanvasEvents.cpp
    src/ui/canvas/CanvasContainer.h
    src/ui/canvas/CanvasContainer.cpp)

# 项目源文件
set(PROJECT_SOURCES
    src/util/mutil.hpp
    src/ui/action/ActionManager.hpp
    src/ui/action/ActionManager.cpp
    src/ui/action/modules/canvas/EditorActions.hpp
    src/ui/action/modules/canvas/EditorActions.cpp
    src/ui/action/modules/canvas/EditorActionHandler.hpp
    src/ui/action/modules/canvas/EditorActionHandler.cpp
    src/ui/action/modules/file/FileActions.hpp
    src/ui/action/modules/file/FileActions.cpp
    src/ui/action/modules/file/FileActionHandler.hpp
    src/ui/action/modules/file/FileActionHandler.cpp
    src/ui/template/AutoResizeTableWidget.h
    src/ui/template/AutoResizeTableWidget.cpp
    src/ui/template/HideableToolWindow.hpp
    src/ui/template/HideableToolWindow.cpp
    src/ui/template/ScalableImageLabel.h
    src/ui/template/ScalableImageLabel.cpp
    src/ui/edit/mapeditor.h
    src/ui/edit/mapeditor.cpp
    src/ui/edit/mapeditormenus.cpp
    src/ui/edit/mapeditorevent.cpp
    src/ui/edit/mapeditorcontrols.cpp
    src/ui/edit/mapeditor.ui
    src/ui/edit/timingmanager.h
    src/ui/edit/timingmanager.cpp
    src/ui/edit/timingmanagercontrolls.cpp
    src/ui/edit/TimingTableUsefulWidgets.cpp
    src/ui/edit/timingmanager.ui
    src/ui/project/projectmanager.h
    src/ui/project/projectmanager.cpp
    src/ui/project/projectmanagercontrols.cpp
    src/ui/project/ProjectService.hpp
    src/ui/project/ProjectService.cpp
    src/ui/project/projectmanager.ui
    src/ui/project/projectconfig.h
    src/ui/project/projectconfig.cpp
    src/ui/project/projectconfig.ui
    src/ui/audio/track/trackmanager.h
    src/ui/audio/track/trackmanager.cpp
    src/ui/audio/track/trackcontrols.cpp
    src/ui/audio/track/trackmanager.ui
    src/ui/audio/callback/AudioPlayCallBack.hpp
    src/ui/audio/callback/AudioPlayCallBack.cpp
    src/ui/audio/graphic/formrender/FormRenderer2D.hpp
    src/ui/audio/graphic/formrender/FormRenderer2D.cpp
    src/ui/audio/graphic/AudioGraphicWidget.h
    src/ui/audio/graphic/AudioGraphicWidget.cpp
    src/ui/audio/graphic/AudioGraphicWidgetEvents.cpp
    src/ui/audio/graphic/AudioGraphicWidgetRender.cpp
    src/ui/audio/control/ProcessChain.hpp
    src/ui/audio/control/audiocontroller.h
    src/ui/audio/control/audiocontroller.cpp
    src/ui/audio/control/audiobasecontrols.cpp
    src/ui/audio/control/audiographcontrols.cpp
    src/ui/audio/control/audioeffectcontrols.cpp
    src/ui/audio/control/audioeqcontrols.cpp
    src/ui/audio/control/audiocontroller.ui
    src/ui/mainwindow.h
    src/ui/mainwindow.cpp
    src/ui/mainwindowcontrols.cpp
    src/ui/mainwindow.ui
    src/main.cpp)

# ui需要包含的路径
include_directories(
  ./src
  ./src/ui
  ./src/ui/canvas
  ./src/ui/template
  ./src/ui/project
  ./src/ui/edit
  ./src/ui/audio/track
  ./src/ui/audio/graphic)

# 创建可执行文件
qt_add_executable(
  MusicMapMaker
  MANUAL_FINALIZATION
  ${MMM_SOURCES}
  ${CANVAS_SOURCES}
  ${PROJECT_SOURCES}
  ${GLSL_FILES}
  ${TS_FILES}
  resources/glsrcs.qrc
  themes/ColinsTheme/Dark/darkstyle.qrc
  themes/ColinsTheme/Light/lightstyle.qrc
  themes/OpenTheme/QtThemeDark/QtThemeDark.qrc
  themes/OpenTheme/QtThemeLight/QtThemeLight.qrc)

add_dependencies(MusicMapMaker IonCachyEngin-dev)

# 链接库
target_link_libraries(
  MusicMapMaker
  PRIVATE spdlog::spdlog
          Qt6::Core
          Qt6::Concurrent
          Qt6::Widgets
          Qt6::OpenGLWidgets
          Qt6::OpenGL
          Qt6::Svg
          Qt6::SvgWidgets
          ICU::uc
          ICU::i18n
          ICU::data
          glm::glm
          Freetype::Freetype
          ${FFTLIB}
          pugixml::pugixml
          IonCachyEngin-dev)

# 设置翻译系统 Qt6 使用现代翻译工具
qt_add_lupdate(MusicMapMaker SOURCES ${PROJECT_SOURCES} TS_FILES ${TS_FILES})

qt_add_lrelease(
  MusicMapMaker
  TS_FILES
  ${TS_FILES}
  QM_FILES_OUTPUT_VARIABLE
  QM_FILES
  OUTPUT_DIRECTORY
  ${CMAKE_CURRENT_BINARY_DIR}/translations)

# 将翻译文件添加到资源系统
qt_add_resources(
  MusicMapMaker
  "translations"
  PREFIX
  "/translations"
  BASE
  ${CMAKE_CURRENT_BINARY_DIR}/translations
  FILES
  ${QM_FILES})

# 设置 macOS 和 Windows 目标属性
set_target_properties(
  MusicMapMaker
  PROPERTIES ${BUNDLE_ID_OPTION} MACOSX_BUNDLE_BUNDLE_VERSION
             ${PROJECT_VERSION} MACOSX_BUNDLE_SHORT_VERSION_STRING
             ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR} MACOSX_BUNDLE
             TRUE WIN32_EXECUTABLE
             TRUE)

# 安装配置
include(GNUInstallDirs)
install(
  TARGETS MusicMapMaker
  BUNDLE DESTINATION .
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Qt6 最终化处理
qt_finalize_executable(MusicMapMaker)
