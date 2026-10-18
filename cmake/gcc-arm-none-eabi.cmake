# =============================================================================
#  CMake 工具链描述文件（告诉 CMake：我要用 arm-none-eabi-gcc 交叉编译）
#
#  这个文件干什么（大白话）：
#    CMake 默认以为你要给 Windows 编一个能双击运行的 .exe。
#    我们其实是要给 STM32 芯片编一个 .hex 固件。
#    这个文件就是把"编译器换成谁、目标系统是什么"提前讲清楚。
#    命令行里通过 -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake 使用。
# =============================================================================

set(CMAKE_SYSTEM_NAME Generic)          # 裸机系统（没有操作系统）
set(CMAKE_SYSTEM_PROCESSOR arm)

# 交叉编译时，"先试着编译一个小程序看看编译器活不活"这一步会失败，
# 因为裸机程序链接需要链接脚本和启动文件。用 STATIC_LIBRARY 绕过这项检查。
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# -----------------------------------------------------------------------------
# 找 arm-none-eabi-gcc
# 顺序：环境变量 ARM_GCC_PATH > 系统 PATH > 常见安装目录
# -----------------------------------------------------------------------------
if(DEFINED ENV{ARM_GCC_PATH})
  set(_ARM_SEARCH_HINTS "$ENV{ARM_GCC_PATH}")
else()
  set(_ARM_SEARCH_HINTS "")
endif()

find_program(ARM_GCC_EXECUTABLE
  NAMES arm-none-eabi-gcc
  HINTS ${_ARM_SEARCH_HINTS}
  PATHS
    "C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/14.2 rel1/bin"
    "C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/13.3 rel1/bin"
    "C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/13.2 rel1/bin"
    "C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/12.3 rel1/bin"
    "C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/12.2 rel1/bin"
    "C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/11.3 rel1/bin"
    "C:/Program Files/Arm GNU Toolchain arm-none-eabi/14.2 rel1/bin"
    "C:/Program Files/ARM GNU Toolchain arm-none-eabi/14.2 rel1/bin"
  DOC "arm-none-eabi-gcc 的完整路径"
)

if(NOT ARM_GCC_EXECUTABLE)
  message(FATAL_ERROR
    "\n\n找不到 arm-none-eabi-gcc！\n"
    "请任选一种方式解决：\n"
    "  1) 执行: winget install Arm.GnuArmEmbeddedToolchain\n"
    "  2) 或设置环境变量 ARM_GCC_PATH 指向工具链的 bin 目录\n"
    "     （例如 C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/14.2 rel1/bin）\n\n")
endif()

get_filename_component(_ARM_BIN_DIR "${ARM_GCC_EXECUTABLE}" DIRECTORY)
message(STATUS "找到 ARM GCC: ${_ARM_BIN_DIR}")

# -----------------------------------------------------------------------------
# 找 ninja（构建加速器，比 make 快很多）
# 如果系统 PATH 里没有，就去我们自己的工具目录找
# -----------------------------------------------------------------------------
find_program(NINJA_EXECUTABLE
  NAMES ninja
  HINTS "C:/Users/邹志伟/EmbeddedTools/ninja"
  DOC "ninja 构建工具路径"
)
if(NINJA_EXECUTABLE)
  set(CMAKE_MAKE_PROGRAM "${NINJA_EXECUTABLE}" CACHE FILEPATH "ninja 路径")
  message(STATUS "找到 ninja: ${NINJA_EXECUTABLE}")
endif()

# 统一指定所有工具，避免 CMake 自己猜
set(CMAKE_C_COMPILER   "${_ARM_BIN_DIR}/arm-none-eabi-gcc.exe")
set(CMAKE_ASM_COMPILER "${_ARM_BIN_DIR}/arm-none-eabi-gcc.exe")
set(CMAKE_CXX_COMPILER "${_ARM_BIN_DIR}/arm-none-eabi-g++.exe")

set(CMAKE_OBJCOPY "${_ARM_BIN_DIR}/arm-none-eabi-objcopy.exe")
set(CMAKE_SIZE    "${_ARM_BIN_DIR}/arm-none-eabi-size.exe")
set(CMAKE_AR      "${_ARM_BIN_DIR}/arm-none-eabi-ar.exe")
set(CMAKE_RANLIB  "${_ARM_BIN_DIR}/arm-none-eabi-ranlib.exe")
set(CMAKE_GDB     "${_ARM_BIN_DIR}/arm-none-eabi-gdb.exe")

# 不要在宿主机（Windows）目录里乱找头文件和库，只用工具链自带的
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
