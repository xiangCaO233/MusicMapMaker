# =============================================================================
# CMake Toolchain File for cross-compiling from macOS to Windows (MinGW-w64)
# =============================================================================

# 1. 设置目标系统信息
#    告诉 CMake 我们要构建的目标平台是 Windows
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# 2. 设置交叉编译器
#    指定 MinGW-w64 工具链中各个编译器的确切名称。
#    CMake 会在系统的 PATH 环境变量中查找这些程序。
#    确保您已经通过 Homebrew (brew install mingw-w64) 或其他方式安装了此工具链。
set(TOOLCHAIN_PREFIX x86_64-w64-mingw32)
set(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)
set(CMAKE_RC_COMPILER  ${TOOLCHAIN_PREFIX}-windres) # 用于处理 Windows 资源文件 (.rc)

# 3. 设置 Sysroot (系统根) - 这是隔离环境的核心
#    - Sysroot 是一个目录，包含了目标平台的所有头文件、库和二进制文件。
#    - 这确保了 CMake 只会使用我们为 Windows 准备的库，而不会误用 macOS 上的库。
#    - 将下面的路径替换为您存放 MinGW-w64 工具链和所有 Windows 依赖库的根目录。
#      (通常是您通过 MSYS2 pacman 安装并复制过来的 mingw64 目录)
set(CMAKE_SYSROOT "/Users/xiang2333/Documents/win-mingw64-toolchain/mingw64")

# 4. 强制 CMake 只在 Sysroot 中查找依赖 - 防止“环境泄漏”
#    这是交叉编译中最关键的设置，可以防止 CMake 找到并使用主机 (macOS)
#    上的任何库 (例如 /opt/homebrew 或 /usr/local 下的库)。
set(CMAKE_FIND_ROOT_PATH ${CMAKE_SYSROOT})

#    - CMAKE_FIND_ROOT_PATH_MODE_PROGRAM:
#      设置为 NEVER，意味着像 moc, uic 这样的主机工具应该在宿主机系统上查找，
#      而不是在 Sysroot 里 (Sysroot 里的 .exe 无法在 macOS 上运行)。
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

#    - CMAKE_FIND_ROOT_PATH_MODE_LIBRARY, INCLUDE, PACKAGE:
#      设置为 ONLY，意味着所有的库 (.a, .lib, .so, .dll)、头文件 (.h)
#      以及 CMake 包配置文件 (FindXXX.cmake, xxxConfig.cmake)
#      都必须且只能在 CMAKE_FIND_ROOT_PATH (即 Sysroot) 中查找。
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# 5. (可选) 设置一些特定于 Windows 的编译和链接标志
#    例如，默认隐藏控制台窗口
# set(CMAKE_EXE_LINKER_FLAGS_INIT "-mwindows")

message(STATUS "Toolchain: Loaded macOS to Windows MinGW-w64.")
message(STATUS "  - Sysroot: ${CMAKE_SYSROOT}")
message(STATUS "  - C++ Compiler: ${CMAKE_CXX_COMPILER}")

