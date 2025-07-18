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

include_directories(./ ./libs/IonCachyEngine/include)

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
             Widgets
             OpenGLWidgets
             OpenGL
             LinguistTools
             Svg
             SvgWidgets)

# 查找日志包
find_package(spdlog REQUIRED)

# 查找字体包
find_package(Freetype REQUIRED)

# 查找pugixml包
find_package(pugixml REQUIRED)

# 字符编码包
find_package(ICU REQUIRED COMPONENTS uc i18n data)

# 设置mmm相关文件
set(MMM_SOURCES)

# 设置翻译文件
set(TS_FILES)

# 设置着色器文件
set(GLSL_FILES)

# 项目源文件
set(PROJECT_SOURCES src/main.cpp)

include_directories(./src ./src/ui)

# 创建可执行文件
qt_add_executable(MusicMapMaker MANUAL_FINALIZATION ${PROJECT_SOURCES}
  src/ui/mainwindow.h src/ui/mainwindow.cpp src/ui/mainwindow.ui)

add_dependencies(MusicMapMaker IonCachyEngin-dev)

# 链接库
target_link_libraries(
  MusicMapMaker
  PRIVATE spdlog::spdlog
          Qt${QT_VERSION_MAJOR}::Core
          Qt${QT_VERSION_MAJOR}::Widgets
          Qt${QT_VERSION_MAJOR}::OpenGLWidgets
          Qt${QT_VERSION_MAJOR}::OpenGL
          Qt${QT_VERSION_MAJOR}::Svg
          Qt${QT_VERSION_MAJOR}::SvgWidgets
          ICU::uc
          ICU::i18n
          ICU::data
          Freetype::Freetype
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
