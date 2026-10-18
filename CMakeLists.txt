# =============================================================================
#  STM32F103 + 标准库（StdPeriph）CMake 构建脚本
#  适用于江协科技教程的工程结构：Start / Library / System / User
# =============================================================================
#
#  为什么用它：
#    Keil 的 .uvprojx 是私有格式，只有 Keil 能读。
#    换到 VSCode 就得有一个"公共格式"的构建描述，CMake 就是这个角色。
#
#  怎么用（在 VSCode 里通常点一下按钮就行，命令行等价于下面两步）：
#    cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
#    cmake --build build
#
#  产物（都在 build/ 目录里）：
#    firmware.elf   带调试信息，给调试器（GDB）用
#    firmware.hex   给 ST-LINK Utility 烧录用
#    firmware.bin   纯二进制，给串口 ISP 烧录用
# =============================================================================

cmake_minimum_required(VERSION 3.20)

# 工程名固定叫 firmware，这样每个实验都输出同样的文件名，
# 不用每建一个工程就改一次名字
project(firmware LANGUAGES C ASM)

# -----------------------------------------------------------------------------
# 1. 自动扫描源码目录
#
#    为什么要自动扫描：
#      江协教程的工程是 Start / Library / System / User 四个目录，
#      但自己写的工程常常多一个 Hardware/ 放驱动。
#      这里不去硬编码目录名，而是把工程下所有一级子目录扫一遍，
#      哪个目录里有 .c 就加进来编译，有 .h 就加进头文件搜索路径。
#      以后你新建一个 Motor/ 目录放电机驱动，不用回来改这个文件。
#
#    CONFIGURE_DEPENDS 的作用：新增/删除 .c 文件后，ninja 会自动重新扫一遍，
#    不需要手动点"重新配置"。
# -----------------------------------------------------------------------------
set(APP_SOURCES "")
set(INCLUDE_DIRS "")

# 这些目录不是源码，扫的时候要跳过（Keil 的中间产物、构建输出）
set(_EXCLUDE_DIR_PATTERN "^(Objects|Listings|DebugConfig|build|cmake|\\.vscode|\\.git)$")

file(GLOB _TOP_ITEMS LIST_DIRECTORIES true "${CMAKE_CURRENT_SOURCE_DIR}/*")
foreach(_item ${_TOP_ITEMS})
    if(NOT IS_DIRECTORY "${_item}")
        continue()
    endif()

    get_filename_component(_dir_name "${_item}" NAME)
    if(_dir_name MATCHES "${_EXCLUDE_DIR_PATTERN}")
        continue()
    endif()

    file(GLOB _c_files CONFIGURE_DEPENDS "${_item}/*.c")
    file(GLOB _h_files "${_item}/*.h")

    # 有头文件的目录，加入 include 搜索路径
    if(_h_files)
        list(APPEND INCLUDE_DIRS "${_item}")
    endif()

    # 有 .c 文件的目录，里面的文件都要参与编译
    if(_c_files)
        list(APPEND APP_SOURCES ${_c_files})
    endif()
endforeach()

# 【坑 1】core_cm3.c 是给 Keil 编译器写的，里面全是 __ASM 这种 ARMCC 私有语法，
#         GCC 编不过。而且 core_cm3.h 里已经有等价的 inline 函数，
#         所以 GCC 工程必须把它排除掉。
list(FILTER APP_SOURCES EXCLUDE REGEX "/core_cm3\\.c$")

# 启动文件用 GCC 版本（Start/ 下那个 Keil 版的 .s 语法完全不同，不能用）
set(STARTUP_FILE "${CMAKE_CURRENT_SOURCE_DIR}/Start/startup_stm32f10x_md_gcc.s")

# 链接脚本（内存地图）
# 不同工程的芯片不一样：F103C8 是 64KB Flash，F103CB 是 128KB Flash，
# 所以不写死文件名，而是自动找工程目录下的 .ld 文件用
file(GLOB PROJECT_LD_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.ld")
list(LENGTH PROJECT_LD_FILES LD_COUNT)
if(LD_COUNT EQUAL 0)
  message(FATAL_ERROR
    "\n\n工程目录里找不到 .ld 链接脚本！\n"
    "请把 STM32F103C8_FLASH.ld（64KB）或 STM32F103CB_FLASH.ld（128KB）放到工程根目录。\n\n")
elseif(LD_COUNT GREATER 1)
  message(WARNING "工程里有多个 .ld 文件，默认使用第一个：${PROJECT_LD_FILES}")
endif()
list(GET PROJECT_LD_FILES 0 LINKER_SCRIPT)
message(STATUS "链接脚本: ${LINKER_SCRIPT}")

# -----------------------------------------------------------------------------
# 2. 生成可执行文件
#    （头文件搜索路径 INCLUDE_DIRS 已经在上面第 1 步自动收集好了）
# -----------------------------------------------------------------------------
add_executable(${PROJECT_NAME}
    ${APP_SOURCES}
    ${STARTUP_FILE}
)

# 强制产物叫 firmware.elf。
# Windows 上交叉编译默认不带后缀，生成的文件就叫 firmware（没有扩展名），
# 而 launch.json 里调试器是按 build/firmware.elf 找的，对不上就会报"找不到可执行文件"。
# （注意：这个后缀必须在 target 上设，写在工具链文件里的 CMAKE_EXECUTABLE_SUFFIX
#   会被 CMake 后面的平台初始化覆盖掉。）
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".elf")

target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIRS})

# 这两个宏是标准库的开关：
#   USE_STDPERIPH_DRIVER  -> 让 stm32f10x.h 去包含 stm32f10x_conf.h（外设驱动总开关）
#   STM32F10X_MD          -> 声明这是中容量芯片（64~128KB Flash），
#                            Keil 里是在 Options -> C/C++ 里选的，GCC 下必须手动给
target_compile_definitions(${PROJECT_NAME} PRIVATE
    USE_STDPERIPH_DRIVER
    STM32F10X_MD
)

# -----------------------------------------------------------------------------
# 4. 编译选项
# -----------------------------------------------------------------------------
target_compile_options(${PROJECT_NAME} PRIVATE
    # 只对 C 源文件生效的部分
    $<$<COMPILE_LANGUAGE:C>:
        -std=gnu11
        -Wall
        -fno-common
    >
    # 只对汇编文件生效（启动文件）的部分
    $<$<COMPILE_LANGUAGE:ASM>:
        -x assembler-with-cpp
    >
    # 公共部分：CPU 型号和指令集
    -mcpu=cortex-m3
    -mthumb
    -mfloat-abi=soft
    # 每个函数/变量单独成段，配合链接时的 --gc-sections 把没用的代码删掉
    -ffunction-sections
    -fdata-sections
)

# 调试版和发布版
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${PROJECT_NAME} PRIVATE -Og -g3 -gdwarf-2)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Os -g)
endif()

# -----------------------------------------------------------------------------
# 5. 链接选项
# -----------------------------------------------------------------------------
target_link_options(${PROJECT_NAME} PRIVATE
    -mcpu=cortex-m3
    -mthumb
    -mfloat-abi=soft
    -T${LINKER_SCRIPT}
    # 删掉没有被用到的函数和数据，省 Flash
    -Wl,--gc-sections
    # 生成 map 文件，看内存占用最有用；出链接错误时也靠它排查
    # 【坑 2】这里故意只写文件名不写绝对路径。因为工程放在含中文的目录下
    #        （比如桌面/3_1LED闪烁），arm-none-eabi-objcopy 这类工具解析
    #        命令行里的中文路径会乱码，报 "No such file or directory"。
    #        只传纯英文文件名，让工具在当前目录里找，就能绕开这个问题。
    -Wl,-Map=${PROJECT_NAME}.map
    -Wl,--print-memory-usage
    # 链接脚本里 .data 段是"存在 Flash、运行在 RAM"，ld 会报 RWX 权限警告，
    # 这是 Cortex-M 的正常布局，不是错误，直接关掉这条警告
    -Wl,--no-warn-rwx-segments
    # 用精简版 C 库（体积小很多），并给系统调用提供空实现，
    # 否则链接时会报 _write / _sbrk 之类 undefined reference
    --specs=nano.specs
    --specs=nosys.specs
)

# -----------------------------------------------------------------------------
# 6. 编译完成后自动生成 hex 和 bin
#    注意：这里全部用相对文件名 + WORKING_DIRECTORY，
#    原因跟上面 -Map 一样——绝对路径里的中文会让 objcopy 找不到文件
# -----------------------------------------------------------------------------
# 注意 $<TARGET_FILE_NAME:...> 只会展开成 "firmware.elf" 这种纯英文文件名，
# 不带目录，所以不会碰上中文路径乱码；同时又能自动跟着后缀变化走。
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE_NAME:${PROJECT_NAME}> ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE_NAME:${PROJECT_NAME}> ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE_NAME:${PROJECT_NAME}>
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "生成 hex / bin 并统计内存占用"
)
