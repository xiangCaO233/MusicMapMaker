cmake_minimum_required(VERSION 3.28)

project(
  MusicMapMaker
  VERSION 0.1
  LANGUAGES CXX)

add_subdirectory(./libs/xaudiolib)
include_directories(./ ./libs/xaudiolib/include)

# 启用自动处理 Qt 特性
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

# 查找 Qt 包
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets OpenGLWidgets
                                                  OpenGL LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED
             COMPONENTS Core Widgets OpenGLWidgets OpenGL LinguistTools)

# 查找日志包
find_package(spdlog REQUIRED)

# 设置翻译文件
set(TS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/translations/MusicMapMaker_zh_CN.ts
             ${CMAKE_CURRENT_SOURCE_DIR}/translations/MusicMapMaker_en_US.ts)
# 设置着色器文件
set(GLSL_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/glsl/fragmentshader-static.glsl.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/glsl/vertexshader-static.glsl.vert)

# 项目源文件
set(PROJECT_SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/mainwindow.ui
    src/canvas/GLCanvas.h
    src/canvas/GLCanvas.cpp
    src/canvas/renderer/RenderCommand.h
    src/canvas/renderer/static/StaticRenderer.h
    src/canvas/renderer/static/StaticRenderer.cpp
		src/canvas/renderer/dynamic/DynamicRenderer.h
    src/canvas/renderer/dynamic/DynamicRenderer.cpp
		src/canvas/renderer/AbstractRenderer.h
    src/canvas/renderer/AbstractRenderer.cpp
		src/canvas/renderer/RendererManager.h
    src/canvas/renderer/RendererManager.cpp
    src/canvas/texture/TexturePool.h
    src/canvas/texture/TexturePool.cpp
    src/canvas/texture/BaseTexturePool.h
    src/canvas/texture/BaseTexturePool.cpp
    src/canvas/texture/atlas/TextureAtlas.h
    src/canvas/texture/atlas/TextureAtlas.cpp
		src/canvas/texture/atlas/Rect.h
		src/canvas/texture/atlas/MaxRectsBinPack.h
		src/canvas/texture/atlas/MaxRectsBinPack.cpp
		src/canvas/texture/array/TextureArray.h
		src/canvas/texture/array/TextureArray.cpp
    src/log/colorful-log.h
    src/log/colorful_log.cpp
    ${GLSL_FILES}
    ${TS_FILES})

include_directories(./src/canvas ./src/log)

# 创建可执行文件
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
  qt_add_executable(MusicMapMaker MANUAL_FINALIZATION ${PROJECT_SOURCES}
                    resources/glsrcs.qrc)
else()
  if(ANDROID)
    add_library(MusicMapMaker SHARED ${PROJECT_SOURCES})
  else()
    add_executable(MusicMapMaker ${PROJECT_SOURCES})
  endif()
endif()

# 链接 Qt 库
target_link_libraries(
  MusicMapMaker
  PRIVATE spdlog::spdlog Qt${QT_VERSION_MAJOR}::Core
          Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::OpenGLWidgets
          Qt${QT_VERSION_MAJOR}::OpenGL xaudio)

# 设置翻译系统
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
  # Qt6 使用现代翻译工具
  qt_add_lupdate(MusicMapMaker SOURCES ${PROJECT_SOURCES} TS_FILES ${TS_FILES})

  qt_add_lrelease(
    MusicMapMaker
    TS_FILES
    ${TS_FILES}
    QM_FILES_OUTPUT_VARIABLE
    QM_FILES
    OUTPUT_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/translations)
else()
  # Qt5 使用旧版翻译工具
  qt5_add_translation(QM_FILES ${TS_FILES})
endif()

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
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.MusicMapMaker)
endif()

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
if(QT_VERSION_MAJOR EQUAL 6)
  qt_finalize_executable(MusicMapMaker)
endif()
