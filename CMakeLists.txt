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
endif(APPLE)

# 查找字体包
find_package(Freetype REQUIRED)

# 查找pugixml包
find_package(pugixml REQUIRED)

# 字符编码包
find_package(ICU REQUIRED COMPONENTS uc i18n data)

# 设置mmm相关文件
set(MMM_SOURCES
    src/mmm/obj/Note.hpp src/mmm/obj/Hold.hpp src/mmm/obj/Slide.hpp
    src/mmm/obj/Composite.hpp src/mmm/NoteCollection.hpp src/mmm/map/MMap.hpp)

# 设置翻译文件
set(TS_FILES)

# 设置着色器文件
set(GLSL_FILES
    resources/glsl/canvas/quad_fshader.glsl.frag
    resources/glsl/canvas/quad_vshader.glsl.vert
    resources/glsl/spectro_fshader.glsl.frag
    resources/glsl/spectro_vshader.glsl.vert
    resources/glsl/wavgraph_fshader.glsl.frag
    resources/glsl/wavgraph_vshader.glsl.vert)

set(CANVAS_SOURCES
    src/ui/canvas/render/quad/QuadData.hpp
    src/ui/canvas/render/texture/TexMode.hpp
    src/ui/canvas/render/texture/TextureInfo.hpp
    src/ui/canvas/render/texture/TexturePool.hpp
    src/ui/canvas/render/texture/TexturePool.cpp
    src/ui/canvas/render/Renderer2D.hpp
    src/ui/canvas/render/Renderer2D.cpp
    src/ui/canvas/FrameRateCounter.hpp
    src/ui/canvas/FrameRateCounter.cpp
    src/ui/canvas/GLCanvas.cpp
    src/ui/canvas/GLCanvas.hpp
    src/ui/canvas/GLCanvasEvents.cpp
    src/ui/canvas/CanvasContainer.h
    src/ui/canvas/CanvasContainer.cpp)

# 项目源文件
set(PROJECT_SOURCES
    src/util/mutil.hpp
    src/ui/template/AutoResizeTableWidget.h
    src/ui/template/AutoResizeTableWidget.cpp
    src/ui/template/HideableToolWindow.hpp
    src/ui/template/HideableToolWindow.cpp
    src/ui/template/ScalableImageLabel.h
    src/ui/template/ScalableImageLabel.cpp
    src/ui/edit/mapeditor.h
    src/ui/edit/mapeditor.cpp
    src/ui/edit/mapeditor.ui
    src/ui/project/projectmanager.h
    src/ui/project/projectmanager.cpp
    src/ui/project/projectmanager.ui
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
include_directories(./src ./src/ui ./src/ui/canvas ./src/ui/template
                    ./src/ui/audio/graphic)

# 创建可执行文件
qt_add_executable(
  MusicMapMaker
  MANUAL_FINALIZATION
  ${MMM_SOURCES}
  ${CANVAS_SOURCES}
  ${PROJECT_SOURCES}
  ${GLSL_FILES}
  resources/glsrcs.qrc)

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
