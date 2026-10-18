#ifndef __MOTOR_H
#define __MOTOR_H

#include "stm32f10x.h"

/*
 * Motor —— 直流风扇电机驱动（TB6612/L9110 类驱动芯片）
 * 方向脚：PA4、PA5；调速脚：PA0（PWM，来自 TIM2 通道 1）
 */

void Motor_Init(void);
void Motor_SetSpeed(int8_t Speed);

#endif
