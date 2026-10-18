/**
  ******************************************************************************
  * @file    startup_stm32f10x_md_gcc.s
  * @brief   GCC 版启动文件（GNU 汇编语法）
  *
  * 为什么需要这个文件：
  *   Start/startup_stm32f10x_md.s 是给 Keil 的 armasm 汇编器用的语法，
  *   GCC 的汇编器（GNU as）完全不认，一编译就是一堆语法错误。
  *   所以必须换成这个 GCC 版本的启动文件。
  *
  * 这个东西到底干什么（用大白话讲）：
  *   芯片一上电，硬件会去 Flash 最开头（0x08000000）读两个东西：
  *     第 1 个字 = 栈顶地址（程序用内存的起点）
  *     第 2 个字 = 复位后第一条指令的地址
  *   紧接着是一张表，叫"中断向量表"，第 N 项就是第 N 号异常/中断来的时候该跳到哪。
  *   这个文件就是铺这张表，并且定义复位后要做的第一件事（Reset_Handler）：
  *     把已初始化的全局变量从 Flash 拷到 RAM -> 把未初始化的全局变量清零
  *     -> 初始化 C 库 -> 跳进 main()
  *
  * 对应芯片：STM32F103CB（中容量 MD），43 个外设中断
  ******************************************************************************
  */

  .syntax unified
  .cpu cortex-m3
  .fpu softvfp
  .thumb

.global g_pfnVectors
.global Default_Handler

/* 这几个符号由链接脚本（.ld 文件）提供，这里只是引用 */
.word _sidata          /* .data 段在 Flash 里的起始地址 */
.word _sdata           /* .data 段在 RAM 里的起始地址 */
.word _edata           /* .data 段在 RAM 里的结束地址 */
.word _sbss            /* .bss 段起始地址 */
.word _ebss            /* .bss 段结束地址 */

/**
  * 复位处理函数：芯片上电 / 按下复位键后执行的第一段代码
  * 参数：无
  * 返回：不会返回（最后跳进 main，main 是死循环）
  */
  .section .text.Reset_Handler
  .weak Reset_Handler
  .type Reset_Handler, %function
Reset_Handler:

  /* ---------- 第 1 步：把 .data 段从 Flash 搬到 RAM ---------- */
  ldr r0, =_sdata        /* r0 = RAM 里的目标起始地址 */
  ldr r1, =_edata        /* r1 = RAM 里的目标结束地址 */
  ldr r2, =_sidata       /* r2 = Flash 里的源起始地址 */
  movs r3, #0            /* r3 = 已经搬了多少字节，从 0 开始 */
  b LoopCopyDataInit

CopyDataInit:
  ldr r4, [r2, r3]       /* 从 Flash 读一个字 */
  str r4, [r0, r3]       /* 写到 RAM 对应位置 */
  adds r3, r3, #4        /* 偏移加 4 字节 */

LoopCopyDataInit:
  adds r4, r0, r3        /* r4 = 当前写到哪了 */
  cmp r4, r1             /* 到结尾了吗 */
  bcc CopyDataInit       /* 没到就继续搬 */

  /* ---------- 第 2 步：把 .bss 段全部清零 ---------- */
  ldr r2, =_sbss
  ldr r4, =_ebss
  movs r3, #0
  b LoopFillZerobss

FillZerobss:
  str r3, [r2]
  adds r2, r2, #4

LoopFillZerobss:
  cmp r2, r4
  bcc FillZerobss

  /* ---------- 第 3 步：初始化系统时钟 ---------- */
  /* SystemInit() 在 Start/system_stm32f10x.c 里，把 72MHz 时钟配好 */
  bl SystemInit

  /* ---------- 第 4 步：初始化 C 库（newlib） ---------- */
  bl __libc_init_array

  /* ---------- 第 5 步：进 main ---------- */
  bl main
  bx lr
  .size Reset_Handler, .-Reset_Handler

/**
  * 默认的中断处理函数
  * 任何一个中断如果用户没写处理函数，就会跑到这里原地死等。
  * 调试的时候如果程序停在这，说明进了一个没人处理的中断。
  */
  .section .text.Default_Handler,"ax",%progbits
Default_Handler:
Infinite_Loop:
  b Infinite_Loop
  .size Default_Handler, .-Default_Handler

/**
  * 中断向量表
  * 顺序绝对不能改！改了就会"中断串位"（比如按按键结果跑进了串口中断）
  * 前 16 项是内核异常（Cortex-M3 规定死的），后面 43 项是 STM32F103 中容量的外设中断
  */
  .section .isr_vector,"a",%progbits
  .type g_pfnVectors, %object
g_pfnVectors:

  .word _estack                  /* 第 0 项：栈顶地址 */
  .word Reset_Handler            /* 第 1 项：复位 */
  .word NMI_Handler              /* 2  NMI */
  .word HardFault_Handler        /* 3  硬件错误 */
  .word MemManage_Handler        /* 4  内存管理错误 */
  .word BusFault_Handler         /* 5  总线错误 */
  .word UsageFault_Handler       /* 6  用法错误 */
  .word 0                        /* 7  保留 */
  .word 0                        /* 8  保留 */
  .word 0                        /* 9  保留 */
  .word 0                        /* 10 保留 */
  .word SVC_Handler              /* 11 系统服务调用 */
  .word DebugMon_Handler         /* 12 调试监控 */
  .word 0                        /* 13 保留 */
  .word PendSV_Handler           /* 14 可挂起系统服务 */
  .word SysTick_Handler          /* 15 系统滴答定时器 */

  /* ---------------- 以下 43 项是 STM32F103 中容量的外设中断 ---------------- */
  .word WWDG_IRQHandler                  /* 16 窗口看门狗 */
  .word PVD_IRQHandler                   /* 17 电源电压检测 */
  .word TAMPER_IRQHandler                /* 18 侵入检测 */
  .word RTC_IRQHandler                   /* 19 实时时钟 */
  .word FLASH_IRQHandler                 /* 20 Flash 操作 */
  .word RCC_IRQHandler                   /* 21 时钟 */
  .word EXTI0_IRQHandler                 /* 22 外部中断线 0 */
  .word EXTI1_IRQHandler                 /* 23 外部中断线 1 */
  .word EXTI2_IRQHandler                 /* 24 外部中断线 2 */
  .word EXTI3_IRQHandler                 /* 25 外部中断线 3 */
  .word EXTI4_IRQHandler                 /* 26 外部中断线 4 */
  .word DMA1_Channel1_IRQHandler         /* 27 DMA1 通道 1 */
  .word DMA1_Channel2_IRQHandler         /* 28 DMA1 通道 2 */
  .word DMA1_Channel3_IRQHandler         /* 29 DMA1 通道 3 */
  .word DMA1_Channel4_IRQHandler         /* 30 DMA1 通道 4 */
  .word DMA1_Channel5_IRQHandler         /* 31 DMA1 通道 5 */
  .word DMA1_Channel6_IRQHandler         /* 32 DMA1 通道 6 */
  .word DMA1_Channel7_IRQHandler         /* 33 DMA1 通道 7 */
  .word ADC1_2_IRQHandler                /* 34 ADC1 和 ADC2 */
  .word USB_HP_CAN1_TX_IRQHandler        /* 35 USB 高优先级 / CAN1 发送 */
  .word USB_LP_CAN1_RX0_IRQHandler       /* 36 USB 低优先级 / CAN1 接收0 */
  .word CAN1_RX1_IRQHandler              /* 37 CAN1 接收1 */
  .word CAN1_SCE_IRQHandler              /* 38 CAN1 状态改变/错误 */
  .word EXTI9_5_IRQHandler               /* 39 外部中断线 9~5 */
  .word TIM1_BRK_IRQHandler              /* 40 定时器1 刹车 */
  .word TIM1_UP_IRQHandler               /* 41 定时器1 更新 */
  .word TIM1_TRG_COM_IRQHandler          /* 42 定时器1 触发/通信 */
  .word TIM1_CC_IRQHandler               /* 43 定时器1 捕获比较 */
  .word TIM2_IRQHandler                  /* 44 定时器2 */
  .word TIM3_IRQHandler                  /* 45 定时器3 */
  .word TIM4_IRQHandler                  /* 46 定时器4 */
  .word I2C1_EV_IRQHandler               /* 47 I2C1 事件 */
  .word I2C1_ER_IRQHandler               /* 48 I2C1 错误 */
  .word I2C2_EV_IRQHandler               /* 49 I2C2 事件 */
  .word I2C2_ER_IRQHandler               /* 50 I2C2 错误 */
  .word SPI1_IRQHandler                  /* 51 SPI1 */
  .word SPI2_IRQHandler                  /* 52 SPI2 */
  .word USART1_IRQHandler                /* 53 串口1 */
  .word USART2_IRQHandler                /* 54 串口2 */
  .word USART3_IRQHandler                /* 55 串口3 */
  .word EXTI15_10_IRQHandler             /* 56 外部中断线 15~10 */
  .word RTCAlarm_IRQHandler              /* 57 RTC 闹钟 */
  .word USBWakeUp_IRQHandler             /* 58 USB 唤醒 */

  .size g_pfnVectors, .-g_pfnVectors

/**
  * 把所有中断处理函数都先"软链接"到 Default_Handler
  * 这样用户只要在 stm32f10x_it.c 里写一个同名函数，就会自动覆盖这里的弱定义。
  * 好处：不用怕漏写某个中断函数导致编译报错。
  */
  .weak      NMI_Handler
  .thumb_set NMI_Handler,Default_Handler

  .weak      HardFault_Handler
  .thumb_set HardFault_Handler,Default_Handler

  .weak      MemManage_Handler
  .thumb_set MemManage_Handler,Default_Handler

  .weak      BusFault_Handler
  .thumb_set BusFault_Handler,Default_Handler

  .weak      UsageFault_Handler
  .thumb_set UsageFault_Handler,Default_Handler

  .weak      SVC_Handler
  .thumb_set SVC_Handler,Default_Handler

  .weak      DebugMon_Handler
  .thumb_set DebugMon_Handler,Default_Handler

  .weak      PendSV_Handler
  .thumb_set PendSV_Handler,Default_Handler

  .weak      SysTick_Handler
  .thumb_set SysTick_Handler,Default_Handler

  .weak      WWDG_IRQHandler
  .thumb_set WWDG_IRQHandler,Default_Handler

  .weak      PVD_IRQHandler
  .thumb_set PVD_IRQHandler,Default_Handler

  .weak      TAMPER_IRQHandler
  .thumb_set TAMPER_IRQHandler,Default_Handler

  .weak      RTC_IRQHandler
  .thumb_set RTC_IRQHandler,Default_Handler

  .weak      FLASH_IRQHandler
  .thumb_set FLASH_IRQHandler,Default_Handler

  .weak      RCC_IRQHandler
  .thumb_set RCC_IRQHandler,Default_Handler

  .weak      EXTI0_IRQHandler
  .thumb_set EXTI0_IRQHandler,Default_Handler

  .weak      EXTI1_IRQHandler
  .thumb_set EXTI1_IRQHandler,Default_Handler

  .weak      EXTI2_IRQHandler
  .thumb_set EXTI2_IRQHandler,Default_Handler

  .weak      EXTI3_IRQHandler
  .thumb_set EXTI3_IRQHandler,Default_Handler

  .weak      EXTI4_IRQHandler
  .thumb_set EXTI4_IRQHandler,Default_Handler

  .weak      DMA1_Channel1_IRQHandler
  .thumb_set DMA1_Channel1_IRQHandler,Default_Handler

  .weak      DMA1_Channel2_IRQHandler
  .thumb_set DMA1_Channel2_IRQHandler,Default_Handler

  .weak      DMA1_Channel3_IRQHandler
  .thumb_set DMA1_Channel3_IRQHandler,Default_Handler

  .weak      DMA1_Channel4_IRQHandler
  .thumb_set DMA1_Channel4_IRQHandler,Default_Handler

  .weak      DMA1_Channel5_IRQHandler
  .thumb_set DMA1_Channel5_IRQHandler,Default_Handler

  .weak      DMA1_Channel6_IRQHandler
  .thumb_set DMA1_Channel6_IRQHandler,Default_Handler

  .weak      DMA1_Channel7_IRQHandler
  .thumb_set DMA1_Channel7_IRQHandler,Default_Handler

  .weak      ADC1_2_IRQHandler
  .thumb_set ADC1_2_IRQHandler,Default_Handler

  .weak      USB_HP_CAN1_TX_IRQHandler
  .thumb_set USB_HP_CAN1_TX_IRQHandler,Default_Handler

  .weak      USB_LP_CAN1_RX0_IRQHandler
  .thumb_set USB_LP_CAN1_RX0_IRQHandler,Default_Handler

  .weak      CAN1_RX1_IRQHandler
  .thumb_set CAN1_RX1_IRQHandler,Default_Handler

  .weak      CAN1_SCE_IRQHandler
  .thumb_set CAN1_SCE_IRQHandler,Default_Handler

  .weak      EXTI9_5_IRQHandler
  .thumb_set EXTI9_5_IRQHandler,Default_Handler

  .weak      TIM1_BRK_IRQHandler
  .thumb_set TIM1_BRK_IRQHandler,Default_Handler

  .weak      TIM1_UP_IRQHandler
  .thumb_set TIM1_UP_IRQHandler,Default_Handler

  .weak      TIM1_TRG_COM_IRQHandler
  .thumb_set TIM1_TRG_COM_IRQHandler,Default_Handler

  .weak      TIM1_CC_IRQHandler
  .thumb_set TIM1_CC_IRQHandler,Default_Handler

  .weak      TIM2_IRQHandler
  .thumb_set TIM2_IRQHandler,Default_Handler

  .weak      TIM3_IRQHandler
  .thumb_set TIM3_IRQHandler,Default_Handler

  .weak      TIM4_IRQHandler
  .thumb_set TIM4_IRQHandler,Default_Handler

  .weak      I2C1_EV_IRQHandler
  .thumb_set I2C1_EV_IRQHandler,Default_Handler

  .weak      I2C1_ER_IRQHandler
  .thumb_set I2C1_ER_IRQHandler,Default_Handler

  .weak      I2C2_EV_IRQHandler
  .thumb_set I2C2_EV_IRQHandler,Default_Handler

  .weak      I2C2_ER_IRQHandler
  .thumb_set I2C2_ER_IRQHandler,Default_Handler

  .weak      SPI1_IRQHandler
  .thumb_set SPI1_IRQHandler,Default_Handler

  .weak      SPI2_IRQHandler
  .thumb_set SPI2_IRQHandler,Default_Handler

  .weak      USART1_IRQHandler
  .thumb_set USART1_IRQHandler,Default_Handler

  .weak      USART2_IRQHandler
  .thumb_set USART2_IRQHandler,Default_Handler

  .weak      USART3_IRQHandler
  .thumb_set USART3_IRQHandler,Default_Handler

  .weak      EXTI15_10_IRQHandler
  .thumb_set EXTI15_10_IRQHandler,Default_Handler

  .weak      RTCAlarm_IRQHandler
  .thumb_set RTCAlarm_IRQHandler,Default_Handler

  .weak      USBWakeUp_IRQHandler
  .thumb_set USBWakeUp_IRQHandler,Default_Handler
