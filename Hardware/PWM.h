#ifndef __PWM_H
#define __PWM_H

#include "stm32f10x.h"

/*
 * PWM —— 直流电机调速用（TIM2 通道 1，输出引脚 PA0）
 * 原工程在 PA2(TIM2_CH3)，因 PA2 让给调试串口 USART2，改到 PA0(TIM2_CH1)
 */

void PWM_Init(void);
void PWM_SetCompare1(uint16_t Compare);

#endif
