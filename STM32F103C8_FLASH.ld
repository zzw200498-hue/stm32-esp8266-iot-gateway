/*
 * STM32F103C8 链接脚本（GCC / ld 用）
 *
 * 这个东西干什么（大白话）：
 *   编译器把每个 .c 编成 .o 之后，还需要一个人来决定"哪段代码放 Flash 哪个位置，
 *   哪个变量放 RAM 哪个位置"。链接脚本就是这张地图。
 *   Keil 里这个地图是藏在软件里的（点一下 Use Memory Layout from Target Dialog 就自动生成），
 *   GCC 下必须自己写一份明文文件。
 *
 * 本文件对应的芯片：STM32F103C8
 *   Flash:  64KB  起始 0x08000000
 *   RAM  :  20KB  起始 0x20000000
 *
 * 换成别的型号时，只需要改下面 MEMORY 里的两个 LENGTH。
 */

ENTRY(Reset_Handler)

/* 内存地图：名字(权限) : 起始地址, 长度 */
MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH =  64K
  RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 20K
}

/* 栈顶地址 = RAM 的最高地址（栈从高往低长）
 * 这个符号被启动文件里的向量表第 0 项引用
 */
_estack = ORIGIN(RAM) + LENGTH(RAM);

/* 给 malloc 用的最小堆和最小栈空间，纯粹用于链接期检查内存够不够 */
_Min_Heap_Size  = 0x200;   /* 512 字节 */
_Min_Stack_Size = 0x400;   /* 1KB */

SECTIONS
{
  /* ---------- 中断向量表：必须放在 Flash 最开头 ---------- */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))   /* KEEP 表示即使没人引用也不能被 --gc-sections 删掉 */
    . = ALIGN(4);
  } >FLASH

  /* ---------- 代码段 ---------- */
  .text :
  {
    . = ALIGN(4);
    *(.text)
    *(.text*)
    *(.glue_7)         /* ARM/Thumb 交互代码，F103 用不到但保留更安全 */
    *(.glue_7t)
    *(.eh_frame)

    KEEP(*(.init))
    KEEP(*(.fini))

    . = ALIGN(4);
    _etext = .;
  } >FLASH

  /* ---------- 只读数据（const 变量、字符串常量） ---------- */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
  } >FLASH

  /* ---------- 异常处理表（C++ 用，C 语言工程基本为空） ---------- */
  .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >FLASH
  .ARM :
  {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } >FLASH

  .preinit_array :
  {
    PROVIDE_HIDDEN(__preinit_array_start = .);
    KEEP(*(.preinit_array*))
    PROVIDE_HIDDEN(__preinit_array_end = .);
  } >FLASH

  .init_array :
  {
    PROVIDE_HIDDEN(__init_array_start = .);
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array*))
    PROVIDE_HIDDEN(__init_array_end = .);
  } >FLASH

  .fini_array :
  {
    PROVIDE_HIDDEN(__fini_array_start = .);
    KEEP(*(SORT(.fini_array.*)))
    KEEP(*(.fini_array*))
    PROVIDE_HIDDEN(__fini_array_end = .);
  } >FLASH

  /* .data 段在 Flash 里的"存放位置"（启动时从这里拷到 RAM） */
  _sidata = LOADADDR(.data);

  /* ---------- 已初始化的全局/静态变量：存在 RAM，初值存在 Flash ---------- */
  .data :
  {
    . = ALIGN(4);
    _sdata = .;            /* 启动文件用：RAM 里的起点 */
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;            /* 启动文件用：RAM 里的终点 */
  } >RAM AT> FLASH

  /* ---------- 未初始化的全局/静态变量：纯 RAM，启动时清零 ---------- */
  .bss :
  {
    . = ALIGN(4);
    _sbss = .;             /* 启动文件用：清零的起点 */
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;             /* 启动文件用：清零的终点 */
  } >RAM

  /* ---------- 检查 RAM 是否还够放堆和栈 ---------- */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE(end = .);
    PROVIDE(_end = .);
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >RAM

  /* 丢弃不需要的段，省 Flash */
  /DISCARD/ :
  {
    libc.a(*)
    libm.a(*)
    libgcc.a(*)
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
