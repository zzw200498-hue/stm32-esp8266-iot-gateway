#ifndef __SERIAL2_H
#define __SERIAL2_H

#include "stm32f10x.h"

/*
 * Serial2 —— ESP8266 专用串口（USART3，PB10=TX / PB11=RX，115200）
 * PA2/PA3(USART2) 让给调试日志，两者互不干扰
 */

void Serial2_Init(void);
void Serial2_SendByte(uint8_t Byte);
void Serial2_SendString(char *String);

#endif
