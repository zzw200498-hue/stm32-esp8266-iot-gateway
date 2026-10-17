#ifndef __SERIAL2_H
#define __SERIAL2_H

#include "stm32f10x.h"

/*
 * Serial2 —— USART2 专用于和 ESP8266 说话（115200）
 * USART1 继续留给电脑调试打印，两个串口互不干扰
 */

void Serial2_Init(void);
void Serial2_SendByte(uint8_t Byte);
void Serial2_SendString(char *String);

#endif
